#include "nfc_eink_screen_i.h"

#include <../lib/flipper_format/flipper_format.h>
#include <../applications/services/storage/storage.h>

#define TAG "NfcEinkScreen"

///TODO: possibly move this closer to save/load functions
#define NFC_EINK_FORMAT_VERSION               (1)
#define NFC_EINK_FILE_HEADER                  "Flipper NFC Eink screen"
#define NFC_EINK_DEVICE_UID_KEY               "UID"
#define NFC_EINK_DEVICE_TYPE_KEY              "NFC type"
#define NFC_EINK_SCREEN_MANUFACTURER_TYPE_KEY "Manufacturer type"
#define NFC_EINK_SCREEN_MANUFACTURER_NAME_KEY "Manufacturer name"
#define NFC_EINK_SCREEN_TYPE_KEY              "Screen type"
#define NFC_EINK_SCREEN_NAME_KEY              "Screen name"
#define NFC_EINK_SCREEN_WIDTH_KEY             "Width"
#define NFC_EINK_SCREEN_HEIGHT_KEY            "Height"
#define NFC_EINK_SCREEN_DATA_BLOCK_SIZE_KEY   "Data block size"
#define NFC_EINK_SCREEN_DATA_TOTAL_KEY        "Data total"
#define NFC_EINK_SCREEN_BLOCK_DATA_KEY        "Block"

///TODO: move this externs to separate files in screen folders for each type
extern const NfcEinkScreenHandlers waveshare_handlers;
extern const NfcEinkScreenHandlers goodisplay_handlers;

typedef struct {
    const NfcEinkScreenHandlers* handlers;
    const char* name;
} NfcEinkScreenManufacturerDescriptor;

static const NfcEinkScreenManufacturerDescriptor manufacturers[NfcEinkManufacturerNum] = {
    [NfcEinkManufacturerWaveshare] =
        {
            .handlers = &waveshare_handlers,
            .name = "Waveshare",
        },
    [NfcEinkManufacturerGoodisplay] =
        {
            .handlers = &goodisplay_handlers,
            .name = "Goodisplay",
        },
};

const char* nfc_eink_screen_get_manufacturer_name(NfcEinkManufacturer manufacturer) {
    furi_assert(manufacturer < NfcEinkManufacturerNum);
    return manufacturers[manufacturer].name;
}

NfcEinkScreen* nfc_eink_screen_alloc(NfcEinkManufacturer manufacturer) {
    furi_check(manufacturer < NfcEinkManufacturerNum);

    NfcEinkScreen* screen = malloc(sizeof(NfcEinkScreen));
    screen->handlers = manufacturers[manufacturer].handlers;

    screen->device = screen->handlers->alloc();

    screen->data = malloc(sizeof(NfcEinkScreenData));

    screen->tx_buf = bit_buffer_alloc(300);
    screen->rx_buf = bit_buffer_alloc(300);
    return screen;
}

static inline uint16_t nfc_eink_screen_calculate_image_size(const NfcEinkScreenInfo* const info) {
    furi_assert(info);
    return info->width * (info->height % 8 == 0 ? (info->height / 8) : (info->height / 8 + 1));
}

void nfc_eink_screen_init(NfcEinkScreen* screen, NfcEinkScreenType type) {
    furi_assert(type != NfcEinkScreenTypeUnknown);
    furi_assert(type < NfcEinkScreenTypeNum);

    NfcEinkScreenData* data = screen->data;
    NfcEinkScreenDevice* device = screen->device;

    const NfcEinkScreenInfo* info = nfc_eink_descriptor_get_by_type(type);
    memcpy(&data->base, info, sizeof(NfcEinkScreenInfo));

    data->image_size = nfc_eink_screen_calculate_image_size(info);

    device->block_total = data->image_size / info->data_block_size;
    if(data->image_size % info->data_block_size != 0) device->block_total += 1;
    size_t memory_size = device->block_total * data->base.data_block_size;

    data->image_data = malloc(memory_size);
    ///TODO: this can be used as a config option, which will define the initial color of the screen
    // memset(data->image_data, 0xFF, data->image_size);
}

void nfc_eink_screen_free(NfcEinkScreen* screen) {
    furi_check(screen);

    screen->handlers->free(screen->device);
    free(screen->device);

    NfcEinkScreenData* data = screen->data;
    free(data->image_data);
    free(data);

    bit_buffer_free(screen->tx_buf);
    bit_buffer_free(screen->rx_buf);
    screen->handlers = NULL;
    free(screen);
}

void nfc_eink_screen_set_callback(
    NfcEinkScreen* screen,
    NfcEinkScreenEventCallback event_callback,
    NfcEinkScreenEventContext event_context) {
    furi_assert(screen);
    furi_assert(event_callback);
    screen->event_callback = event_callback;
    screen->event_context = event_context;
}

NfcDevice* nfc_eink_screen_get_nfc_device(const NfcEinkScreen* screen) {
    furi_assert(screen);
    return screen->device->nfc_device;
}

const NfcEinkScreenInfo* nfc_eink_screen_get_image_info(const NfcEinkScreen* screen) {
    furi_assert(screen);
    return &screen->data->base;
}

const uint8_t* nfc_eink_screen_get_image_data(const NfcEinkScreen* screen) {
    furi_assert(screen);
    return screen->data->image_data;
}

uint16_t nfc_eink_screen_get_image_size(const NfcEinkScreen* screen) {
    furi_assert(screen);
    return screen->data->image_size;
}

uint16_t nfc_eink_screen_get_received_size(const NfcEinkScreen* screen) {
    furi_assert(screen);
    return screen->device->received_data;
}

NfcGenericCallback nfc_eink_screen_get_nfc_callback(const NfcEinkScreen* screen, NfcMode mode) {
    furi_assert(screen);
    furi_assert(mode < NfcModeNum);

    const NfcEinkScreenHandlers* handlers = screen->handlers;
    return (mode == NfcModeListener) ? handlers->listener_callback : handlers->poller_callback;
}

void nfc_eink_screen_get_progress(const NfcEinkScreen* screen, size_t* current, size_t* total) {
    furi_assert(screen);
    furi_assert(current);
    furi_assert(total);

    *current = screen->device->block_current;
    *total = screen->device->block_total;
}

const char* nfc_eink_screen_get_name(const NfcEinkScreen* screen) {
    furi_assert(screen);
    return screen->data->base.name;
}

bool nfc_eink_screen_load(const char* file_path, NfcEinkScreen** screen) {
    furi_assert(screen);
    furi_assert(file_path);

    Storage* storage = furi_record_open(RECORD_STORAGE);
    FlipperFormat* ff = flipper_format_buffered_file_alloc(storage);

    bool loaded = false;
    NfcEinkScreen* scr = NULL;
    do {
        if(!flipper_format_buffered_file_open_existing(ff, file_path)) break;

        uint32_t manufacturer = 0;
        if(!flipper_format_read_uint32(ff, NFC_EINK_SCREEN_MANUFACTURER_TYPE_KEY, &manufacturer, 1))
            break;
        scr = nfc_eink_screen_alloc(manufacturer);

        uint32_t screen_type = 0;
        if(!flipper_format_read_uint32(ff, NFC_EINK_SCREEN_TYPE_KEY, &screen_type, 1)) break;
        nfc_eink_screen_init(scr, screen_type);

        uint32_t buf = 0;
        if(!flipper_format_read_uint32(ff, NFC_EINK_SCREEN_DATA_TOTAL_KEY, &buf, 1)) break;
        scr->data->image_size = buf;

        FuriString* temp_str = furi_string_alloc();
        bool block_loaded = false;
        for(uint16_t i = 0; i < scr->data->image_size; i += scr->data->base.data_block_size) {
            furi_string_printf(temp_str, "%s %d", NFC_EINK_SCREEN_BLOCK_DATA_KEY, i);
            block_loaded = flipper_format_read_hex(
                ff,
                furi_string_get_cstr(temp_str),
                &scr->data->image_data[i],
                scr->data->base.data_block_size);
            if(!block_loaded) break;
        }
        furi_string_free(temp_str);

        loaded = block_loaded;
        *screen = scr;
    } while(false);

    flipper_format_free(ff);
    furi_record_close(RECORD_STORAGE);
    if(!loaded) {
        nfc_eink_screen_free(scr);
        *screen = NULL;
    }
    return loaded;
}

bool nfc_eink_screen_load_info(const char* file_path, const NfcEinkScreenInfo** info) {
    furi_assert(info);
    furi_assert(file_path);
    bool loaded = false;
    Storage* storage = furi_record_open(RECORD_STORAGE);
    FlipperFormat* ff = flipper_format_buffered_file_alloc(storage);

    do {
        if(!flipper_format_buffered_file_open_existing(ff, file_path)) break;

        uint32_t tmp = 0;
        if(!flipper_format_read_uint32(ff, NFC_EINK_SCREEN_MANUFACTURER_TYPE_KEY, &tmp, 1)) break;
        NfcEinkManufacturer manufacturer = tmp;

        if(!flipper_format_read_uint32(ff, NFC_EINK_SCREEN_TYPE_KEY, &tmp, 1)) break;
        NfcEinkScreenType screen_type = tmp;
        if(screen_type == NfcEinkScreenTypeUnknown || screen_type == NfcEinkScreenTypeNum) {
            FURI_LOG_E(TAG, "Loaded screen type is invalid");
            break;
        }

        uint32_t width = 0;
        if(!flipper_format_read_uint32(ff, NFC_EINK_SCREEN_WIDTH_KEY, &width, 1)) break;

        uint32_t height = 0;
        if(!flipper_format_read_uint32(ff, NFC_EINK_SCREEN_HEIGHT_KEY, &height, 1)) break;

        uint32_t block_data_size = 0;
        if(!flipper_format_read_uint32(
               ff, NFC_EINK_SCREEN_DATA_BLOCK_SIZE_KEY, &block_data_size, 1))
            break;

        const NfcEinkScreenInfo* inf_tmp = nfc_eink_descriptor_get_by_type(screen_type);

        if(inf_tmp->screen_manufacturer != manufacturer) {
            FURI_LOG_E(TAG, "Loaded manufacturer doesn't match with info field");
            break;
        }

        if(inf_tmp->width != width || inf_tmp->height != height) {
            FURI_LOG_E(TAG, "Loaded screen size doesn't match with info field");
            break;
        }

        if(inf_tmp->data_block_size != block_data_size) {
            FURI_LOG_E(TAG, "Loaded block_data_size doesn't match with info field");
            break;
        }

        *info = inf_tmp;
        loaded = true;
    } while(false);

    flipper_format_free(ff);
    furi_record_close(RECORD_STORAGE);

    return loaded;
}

bool nfc_eink_screen_load_data(
    const char* file_path,
    NfcEinkScreen* destination,
    const NfcEinkScreenInfo* source_info) {
    furi_assert(destination);
    furi_assert(destination->data->image_data);
    furi_assert(source_info);

    bool loaded = false;
    Storage* storage = furi_record_open(RECORD_STORAGE);
    FlipperFormat* ff = flipper_format_buffered_file_alloc(storage);

    do {
        if(!flipper_format_buffered_file_open_existing(ff, file_path)) break;

        NfcEinkScreenData* data = destination->data;

        FuriString* temp_str = furi_string_alloc();
        const uint16_t src_img_size = nfc_eink_screen_calculate_image_size(source_info);

        const uint16_t image_size = MIN(data->image_size, src_img_size);

        bool block_loaded = false;
        for(uint16_t i = 0; i < image_size; i += source_info->data_block_size) {
            furi_string_printf(temp_str, "%s %d", NFC_EINK_SCREEN_BLOCK_DATA_KEY, i);
            block_loaded = flipper_format_read_hex(
                ff,
                furi_string_get_cstr(temp_str),
                &data->image_data[i],
                source_info->data_block_size);
            if(!block_loaded) break;
        }
        furi_string_free(temp_str);

        loaded = block_loaded;
    } while(false);

    flipper_format_free(ff);
    furi_record_close(RECORD_STORAGE);

    return loaded;
}

bool nfc_eink_screen_save(const NfcEinkScreen* screen, const char* file_path) {
    furi_assert(screen);
    furi_assert(file_path);

    Storage* storage = furi_record_open(RECORD_STORAGE);

    FlipperFormat* ff = flipper_format_buffered_file_alloc(storage);
    FuriString* temp_str = furi_string_alloc();

    bool saved = false;
    do {
        if(!flipper_format_buffered_file_open_always(ff, file_path)) break;

        // Write header
        if(!flipper_format_write_header_cstr(ff, NFC_EINK_FILE_HEADER, NFC_EINK_FORMAT_VERSION))
            break;

        // Write device type
        NfcProtocol protocol = nfc_device_get_protocol(screen->device->nfc_device);
        if(!flipper_format_write_string_cstr(
               ff, NFC_EINK_DEVICE_TYPE_KEY, nfc_device_get_protocol_name(protocol)))
            break;

        // Write UID
        furi_string_printf(
            temp_str, "%s size can be different depending on type", NFC_EINK_DEVICE_UID_KEY);
        if(!flipper_format_write_comment(ff, temp_str)) break;

        size_t uid_len;
        const uint8_t* uid = nfc_device_get_uid(screen->device->nfc_device, &uid_len);
        if(!flipper_format_write_hex(ff, NFC_EINK_DEVICE_UID_KEY, uid, uid_len)) break;

        // Write manufacturer type
        uint32_t buf = screen->data->base.screen_manufacturer;
        if(!flipper_format_write_uint32(ff, NFC_EINK_SCREEN_MANUFACTURER_TYPE_KEY, &buf, 1)) break;

        //uint32_t buf = screen->data->base.screen_manufacturer;
        //if(!flipper_format_write_uint32(ff, NFC_EINK_SCREEN_MANUFACTURER_NAME_KEY, &buf, 1)) break;

        // Write screen type
        buf = screen->data->base.screen_type;
        if(!flipper_format_write_uint32(ff, NFC_EINK_SCREEN_TYPE_KEY, &buf, 1)) break;

        // Write screen name
        if(!flipper_format_write_string_cstr(ff, NFC_EINK_SCREEN_NAME_KEY, screen->data->base.name))
            break;

        // Write screen width
        buf = screen->data->base.width;
        if(!flipper_format_write_uint32(ff, NFC_EINK_SCREEN_WIDTH_KEY, &buf, 1)) break;

        // Write screen height
        buf = screen->data->base.height;
        if(!flipper_format_write_uint32(ff, NFC_EINK_SCREEN_HEIGHT_KEY, &buf, 1)) break;

        // Write data block size
        furi_string_printf(
            temp_str,
            "%s may be different depending on type",
            NFC_EINK_SCREEN_DATA_BLOCK_SIZE_KEY);
        if(!flipper_format_write_comment(ff, temp_str)) break;

        buf = screen->data->base.data_block_size;
        if(!flipper_format_write_uint32(ff, NFC_EINK_SCREEN_DATA_BLOCK_SIZE_KEY, &buf, 1)) break;

        // Write image total size
        buf = screen->data->image_size;
        if(!flipper_format_write_uint32(ff, NFC_EINK_SCREEN_DATA_TOTAL_KEY, &buf, 1)) break;

        /*   // Write image read size
        buf = screen->device->received_data;
        if(!flipper_format_write_uint32(ff, NFC_EINK_SCREEN_DATA_READ_KEY, &buf, 1)) break; */

        // Write image data
        furi_string_printf(temp_str, "Data");
        if(!flipper_format_write_comment(ff, temp_str)) break;

        bool block_saved = false;
        for(uint16_t i = 0; i < screen->device->received_data;
            i += screen->data->base.data_block_size) {
            furi_string_printf(temp_str, "%s %d", NFC_EINK_SCREEN_BLOCK_DATA_KEY, i);
            block_saved = flipper_format_write_hex(
                ff,
                furi_string_get_cstr(temp_str),
                &screen->data->image_data[i],
                screen->data->base.data_block_size);

            if(!block_saved) break;
        }
        saved = block_saved;
    } while(false);

    flipper_format_free(ff);
    furi_string_free(temp_str);
    furi_record_close(RECORD_STORAGE);

    return saved;
}

bool nfc_eink_screen_delete(const char* file_path) {
    furi_assert(file_path);
    Storage* storage = furi_record_open(RECORD_STORAGE);
    bool deleted = storage_simply_remove(storage, file_path);
    furi_record_close(RECORD_STORAGE);
    return deleted;
}

static void nfc_eink_screen_event_invoke(NfcEinkScreen* instance, NfcEinkScreenEventType type) {
    furi_assert(instance);
    if(instance->event_callback != NULL) {
        instance->event_callback(type, instance->event_context);
    }
}

void nfc_eink_screen_vendor_callback(NfcEinkScreen* instance, NfcEinkScreenEventType type) {
    furi_assert(instance);

    if(type == NfcEinkScreenEventTypeConfigurationReceived) {
        ///TODO: think of throwing this event outside also
        FURI_LOG_D(TAG, "Config received");
        nfc_eink_screen_init(instance, instance->device->screen_type);
    } else
        nfc_eink_screen_event_invoke(instance, type);
}

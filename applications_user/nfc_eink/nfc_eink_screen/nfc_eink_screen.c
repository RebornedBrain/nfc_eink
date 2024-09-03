#include "nfc_eink_screen_i.h"

#include <../lib/flipper_format/flipper_format.h>
#include <../applications/services/storage/storage.h>

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
#define NFC_EINK_SCREEN_DATA_READ_KEY         "Data read"
#define NFC_EINK_SCREEN_BLOCK_DATA_KEY        "Block"
//#include <nfc/protocols/nfc_protocol.h>

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
    screen->rx_buf = bit_buffer_alloc(50);
    return screen;
}

void nfc_eink_screen_init(NfcEinkScreen* screen, NfcEinkScreenType type) {
    furi_assert(type != NfcEinkScreenTypeUnknown);
    furi_assert(type < NfcEinkScreenTypeNum);

    NfcEinkScreenData* data = screen->data;
    NfcEinkScreenDevice* device = screen->device;

    const NfcEinkScreenDescriptor* info = nfc_eink_descriptor_get_by_type(type);
    memcpy(&data->base, info, sizeof(NfcEinkScreenDescriptor));

    data->image_size =
        info->width * (info->height % 8 == 0 ? (info->height / 8) : (info->height / 8 + 1));

    device->block_total = data->image_size / info->data_block_size;
    data->image_data = malloc(data->image_size);
}

void nfc_eink_screen_free(NfcEinkScreen* screen) {
    furi_check(screen);

    screen->handlers->free(screen->device);

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
    return screen->data->received_data;
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
        if(!flipper_format_read_uint32(ff, NFC_EINK_SCREEN_DATA_READ_KEY, &buf, 1)) break;
        scr->data->received_data = buf;

        FuriString* temp_str = furi_string_alloc();
        bool block_loaded = false;
        for(uint16_t i = 0; i < scr->data->received_data; i += scr->data->base.data_block_size) {
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

        // Write image read size
        buf = screen->data->received_data;
        if(!flipper_format_write_uint32(ff, NFC_EINK_SCREEN_DATA_READ_KEY, &buf, 1)) break;

        // Write image data
        furi_string_printf(temp_str, "Data");
        if(!flipper_format_write_comment(ff, temp_str)) break;

        bool block_saved = false;
        for(uint16_t i = 0; i < screen->data->received_data;
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

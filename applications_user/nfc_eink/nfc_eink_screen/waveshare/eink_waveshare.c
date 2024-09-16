#include "eink_waveshare.h"
#include "eink_waveshare_i.h"
#include <nfc/helpers/iso14443_crc.h>

#define TAG "WSH_Listener"

/// TODO: Move this to waveshare context
static uint8_t blocks[16 * 4];

static NfcDevice* eink_waveshare_nfc_device_alloc() {
    //const uint8_t uid[] = {0x46, 0x53, 0x54, 0x5E, 0x31, 0x30, 0x6D}; //FSTN10m
    const uint8_t uid[] = {0x57, 0x53, 0x44, 0x5A, 0x31, 0x30, 0x6D}; //WSDZ10m
    const uint8_t atqa[] = {0x44, 0x00};

    Iso14443_3aData* iso14443_3a_edit_data = iso14443_3a_alloc();
    iso14443_3a_set_uid(iso14443_3a_edit_data, uid, sizeof(uid));
    iso14443_3a_set_sak(iso14443_3a_edit_data, 0);
    iso14443_3a_set_atqa(iso14443_3a_edit_data, atqa);

    NfcDevice* nfc_device = nfc_device_alloc();
    nfc_device_set_data(nfc_device, NfcProtocolIso14443_3a, iso14443_3a_edit_data);

    iso14443_3a_free(iso14443_3a_edit_data);

    const uint8_t block03[] = {
        0x57,
        0x53,
        0x44,
        0xC8,
        0x5A,
        0x31,
        0x30,
        0x6D,
        0x36,
        0,
        0,
        0,
        0x00,
        0x00,
        0x00,
        0x00, //0xE1,
        //0x10,
        //0x6D,
        //0x00,
    };
    memset(blocks, 0, 16 * 4);
    memcpy(&blocks[0], block03, sizeof(block03));

    return nfc_device;
}

static NfcEinkScreenDevice* eink_waveshare_alloc() {
    NfcEinkScreenDevice* device = malloc(sizeof(NfcEinkScreenDevice));
    device->nfc_device = eink_waveshare_nfc_device_alloc();

    NfcEinkWaveshareSpecificContext* ctx = malloc(sizeof(NfcEinkWaveshareSpecificContext));
    ctx->listener_state = NfcEinkWaveshareListenerStateIdle;

    device->screen_context = ctx;
    return device;
}

static void eink_waveshare_free(NfcEinkScreenDevice* instance) {
    furi_assert(instance);
    nfc_device_free(instance->nfc_device);
    free(instance->screen_context);
}

/// TODO: this can be removed
static void eink_waveshare_init(NfcEinkScreenData* data, NfcEinkScreenType generic_type) {
    UNUSED(data);
    UNUSED(generic_type);
    /*     furi_assert(data);
    NfcEinkScreenTypeWaveshare waveshare_type = (NfcEinkScreenTypeWaveshare)generic_type;
    furi_assert(waveshare_type != NfcEinkScreenTypeWaveshareUnknown);
    furi_assert(waveshare_type < NfcEinkScreenTypeWaveshareNum); 

    data->base = waveshare_screens[waveshare_type];
    */
}

static void
    eink_waveshare_parse_config(NfcEinkScreen* screen, const uint8_t* data, uint8_t data_length) {
    //UNUSED(screen);
    UNUSED(data_length);
    //UNUSED(data);

    NfcEinkScreenType type = NfcEinkScreenTypeUnknown;
    uint8_t protocol_type = data[0];

    if(protocol_type == 4 || protocol_type == 2) {
        type = NfcEinkScreenTypeWaveshare2n13inch;
    } else if(protocol_type == 7) {
        type = NfcEinkScreenTypeWaveshare2n9inch;
    }

    screen->device->screen_type = type;
    eink_waveshare_on_config_received(screen);
}

static NfcCommand eink_waveshare_listener_callback(NfcGenericEvent event, void* context) {
    NfcCommand command = NfcCommandContinue;
    NfcEinkScreen* instance = context;
    Iso14443_3aListenerEvent* Iso14443_3a_event = event.event_data;

    NfcEinkWaveshareSpecificContext* ctx = instance->device->screen_context;

    if(Iso14443_3a_event->type == Iso14443_3aListenerEventTypeReceivedData) {
        FURI_LOG_D(TAG, "ReceivedData");
    } else if(Iso14443_3a_event->type == Iso14443_3aListenerEventTypeFieldOff) {
        FURI_LOG_D(TAG, "FieldOff");
        if(ctx->listener_state == NfcEinkWaveshareListenerStateUpdatedSuccefully)
            eink_waveshare_on_done(instance);
        else if(ctx->listener_state != NfcEinkWaveshareListenerStateIdle)
            eink_waveshare_on_target_lost(instance);
        command = NfcCommandStop;
    } else if(Iso14443_3a_event->type == Iso14443_3aListenerEventTypeHalted) {
        FURI_LOG_D(TAG, "Halted");
        if(ctx->listener_state == NfcEinkWaveshareListenerStateUpdatedSuccefully)
            eink_waveshare_on_done(instance);
    } else if(Iso14443_3a_event->type == Iso14443_3aListenerEventTypeReceivedStandardFrame) {
        BitBuffer* buffer = Iso14443_3a_event->data->buffer;

        const uint8_t* data = bit_buffer_get_data(buffer);
        FURI_LOG_D(TAG, "0x%02X, 0x%02X", data[0], data[1]);

        ///TODO: move this to process function
        bit_buffer_reset(instance->tx_buf);
        NfcEinkScreenData* const screen_data = instance->data;
        NfcEinkScreenDevice* const screen_device = instance->device;
        if(data[0] == 0xFF && data[1] == 0xFE) {
            bit_buffer_append_byte(instance->tx_buf, 0xEE);
            bit_buffer_append_byte(instance->tx_buf, 0xFF);
            iso14443_crc_append(Iso14443CrcTypeA, instance->tx_buf);
        } else if(data[0] == 0x30) {
            uint8_t index = data[1];
            bit_buffer_append_bytes(instance->tx_buf, &blocks[index * 4], 4 * 4);
            iso14443_crc_append(Iso14443CrcTypeA, instance->tx_buf);
        } else if(data[0] == 0xA2) {
            uint8_t index = data[1];
            memcpy(&blocks[index * 4], &data[2], 4);
            bit_buffer_append_byte(instance->tx_buf, 0x0A);
            iso14443_crc_append(Iso14443CrcTypeA, instance->tx_buf);
        } else if(data[0] == 0xCD) {
            if(data[1] == 0x00) {
                eink_waveshare_parse_config(instance, data + 2, 1);
                bit_buffer_append_byte(instance->tx_buf, 0x00);
                bit_buffer_append_byte(instance->tx_buf, 0x00);
            } else if(data[1] == 0x04) {
                bit_buffer_append_byte(instance->tx_buf, 0x00);
                bit_buffer_append_byte(instance->tx_buf, 0x00);
                ctx->listener_state = NfcEinkWaveshareListenerStateUpdatedSuccefully;
            } else if(data[1] == 0x0D) {
                bit_buffer_append_byte(instance->tx_buf, 0x00);
                //bit_buffer_append_byte(instance->tx_buf, 0x02); //Some kind of busy status
                bit_buffer_append_byte(instance->tx_buf, 0x00);
                ctx->listener_state = NfcEinkWaveshareListenerStateWaitingForConfig;
                eink_waveshare_on_target_detected(instance);
            } else if(data[1] == 0x08) {
                memcpy(screen_data->image_data + screen_device->received_data, &data[3], data[2]);
                screen_device->received_data += data[2];

                uint16_t last = screen_device->received_data - 1;
                screen_data->image_data[last] &= 0xC0;

                bit_buffer_append_byte(instance->tx_buf, 0x00);
                bit_buffer_append_byte(instance->tx_buf, 0x00);
                ctx->listener_state = NfcEinkWaveshareListenerStateReadingBlocks;
            } else if(data[1] == 0x0A) {
                bit_buffer_append_byte(instance->tx_buf, 0xFF);
                bit_buffer_append_byte(instance->tx_buf, 0x00);
            } else {
                bit_buffer_append_byte(instance->tx_buf, 0x00);
                bit_buffer_append_byte(instance->tx_buf, 0x00);
            }

            iso14443_crc_append(Iso14443CrcTypeA, instance->tx_buf);
        }
        if(bit_buffer_get_size_bytes(instance->tx_buf) > 0)
            nfc_listener_tx(event.instance, instance->tx_buf);
    }

    return command;
}

static NfcCommand eink_waveshare_poller_callback(NfcGenericEvent event, void* context) {
    furi_assert(context);
    UNUSED(event);
    return NfcCommandContinue;
}

const NfcEinkScreenHandlers waveshare_handlers = {
    .alloc = eink_waveshare_alloc,
    .free = eink_waveshare_free,
    .init = eink_waveshare_init,
    .listener_callback = eink_waveshare_listener_callback,
    .poller_callback = eink_waveshare_poller_callback,
};

#include "eink_waveshare.h"
#include "eink_waveshare_i.h"

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
    return nfc_device;
}

static NfcEinkScreenDevice* eink_waveshare_alloc() {
    NfcEinkScreenDevice* device = malloc(sizeof(NfcEinkScreenDevice));
    device->nfc_device = eink_waveshare_nfc_device_alloc();

    NfcEinkWaveshareSpecificContext* ctx = malloc(sizeof(NfcEinkWaveshareSpecificContext));
    ctx->listener_state = NfcEinkWaveshareListenerStateIdle;
    ctx->poller_state = EinkWavesharePollerStateInit;

    const uint8_t block0_3[] = {
        0x57, 0x53, 0x44, 0xC8, 0x5A, 0x31, 0x30, 0x6D, 0x36, 0, 0, 0, 0x00, 0x00, 0x00, 0x00};
    memcpy(ctx->buf, block0_3, sizeof(block0_3));

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

void eink_waveshare_parse_config(NfcEinkScreen* screen, const uint8_t* data, uint8_t data_length) {
    //UNUSED(screen);
    UNUSED(data_length);
    //UNUSED(data);

    NfcEinkScreenType type = NfcEinkScreenTypeUnknown;
    uint8_t protocol_type = data[0];

    if(protocol_type == EinkScreenTypeWaveshare2n13inch) {
        type = NfcEinkScreenTypeWaveshare2n13inch;
    } else if(protocol_type == EinkScreenTypeWaveshare2n9inch) {
        type = NfcEinkScreenTypeWaveshare2n9inch;
    } else if(protocol_type == EinkScreenTypeWaveshare2n7inch) {
        type = NfcEinkScreenTypeWaveshare2n7inch;
    } else if(protocol_type == EinkScreenTypeWaveshare4n2inch) {
        type = NfcEinkScreenTypeWaveshare4n2inch;
    } else if(
        protocol_type == EinkScreenTypeWaveshare7n5inch ||
        protocol_type == EinkScreenTypeWaveshare7n5inchV2) {
        type = NfcEinkScreenTypeWaveshare7n5inch;
    }

    screen->device->screen_type = type;
    eink_waveshare_on_config_received(screen);
}

const NfcEinkScreenHandlers waveshare_handlers = {
    .alloc = eink_waveshare_alloc,
    .free = eink_waveshare_free,
    .init = eink_waveshare_init,
    .listener_callback = eink_waveshare_listener_callback,
    .poller_callback = eink_waveshare_poller_callback,
};

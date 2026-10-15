#include "eink_waveshare_v2_i.h"

/*
 * Grundgeruest des Waveshare-V2-Treibers.
 * Der V2-Tag wird nur BESCHRIEBEN (Poller). Emulieren (Listener) gibt es nicht,
 * deshalb ist listener_callback NULL und V2 erscheint nicht im Emulate-Menue.
 */

static NfcDevice* eink_waveshare_v2_nfc_device_alloc() {
    /* Die Werte werden nur gebraucht, damit die App weiss, dass sie
       ISO14443-4A sprechen muss. Echte Tag-Daten kommen beim Lesen. */
    const uint8_t uid[] = {0x57, 0x53, 0x56, 0x32}; // "WSV2"
    const uint8_t atqa[] = {0x04, 0x00};

    Iso14443_4aData* data_4a = iso14443_4a_alloc();
    iso14443_4a_set_uid(data_4a, uid, sizeof(uid));

    Iso14443_3aData* base_data = iso14443_4a_get_base_data(data_4a);
    iso14443_3a_set_sak(base_data, 0x28);
    iso14443_3a_set_atqa(base_data, atqa);

    NfcDevice* nfc_device = nfc_device_alloc();
    nfc_device_set_data(nfc_device, NfcProtocolIso14443_4a, data_4a);

    iso14443_4a_free(data_4a);
    return nfc_device;
}

static NfcEinkScreenDevice* eink_waveshare_v2_alloc() {
    NfcEinkScreenDevice* device = malloc(sizeof(NfcEinkScreenDevice));
    memset(device, 0, sizeof(NfcEinkScreenDevice));
    device->nfc_device = eink_waveshare_v2_nfc_device_alloc();

    NfcEinkWaveshareV2SpecificContext* ctx = malloc(sizeof(NfcEinkWaveshareV2SpecificContext));
    memset(ctx, 0, sizeof(NfcEinkWaveshareV2SpecificContext));
    ctx->poller_state = EinkWaveshareV2PollerStateAuth;

    device->screen_context = ctx;
    return device;
}

static void eink_waveshare_v2_free(NfcEinkScreenDevice* instance) {
    furi_assert(instance);
    nfc_device_free(instance->nfc_device);
    free(instance->screen_context);
}

const NfcEinkScreenHandlers waveshare_v2_handlers = {
    .alloc = eink_waveshare_v2_alloc,
    .free = eink_waveshare_v2_free,
    .listener_callback = NULL,
    .poller_callback = eink_waveshare_v2_poller_callback,
};

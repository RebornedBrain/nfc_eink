#pragma once
#include "../nfc_eink_screen_i.h"
#include <nfc/protocols/iso14443_3a/iso14443_3a.h>
#include <nfc/protocols/iso14443_3a/iso14443_3a_listener.h>

typedef enum {
    NfcEinkScreenTypeWaveshareUnknown,
    NfcEinkScreenTypeWaveshare2n13inch,
    NfcEinkScreenTypeWaveshare2n9inch,
    //NfcEinkTypeWaveshare4n2inch,

    NfcEinkScreenTypeWaveshareNum
} NfcEinkScreenTypeWaveshare;

#define eink_waveshare_on_done(instance) \
    nfc_eink_screen_event_callback(instance, NfcEinkScreenEventTypeDone)

#define eink_waveshare_on_config_received(instance) \
    nfc_eink_screen_event_callback(instance, NfcEinkScreenEventTypeConfigurationReceived)

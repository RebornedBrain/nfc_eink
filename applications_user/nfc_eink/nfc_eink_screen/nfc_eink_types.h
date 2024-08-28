#pragma once

#include <furi.h>
#include <nfc/nfc.h>
#include <nfc/protocols/nfc_generic_event.h>
#include <nfc/nfc_common.h>
#include <nfc/nfc_device.h>

typedef enum {
    //NfcEinkManufacturerGeneric,
    NfcEinkManufacturerWaveshare,
    NfcEinkManufacturerGoodisplay,

    NfcEinkManufacturerNum
} NfcEinkManufacturer;

typedef enum {
    NfcEinkTypeUnknown
} NfcEinkType;

typedef enum {
    NfcEinkScreenEventTypeTargetDetected,
    NfcEinkScreenEventTypeTargetLost,

    NfcEinkScreenEventTypeConfigurationReceived,
    NfcEinkScreenEventTypeBlockProcessed,
    NfcEinkScreenEventTypeUpdating,

    NfcEinkScreenEventTypeFinish,
    NfcEinkScreenEventTypeFailure,
} NfcEinkScreenEventType;

typedef void (*NfcEinkScreenEventCallback)(NfcEinkScreenEventType type, void* context);

typedef void* NfcEinkScreenEventContext;
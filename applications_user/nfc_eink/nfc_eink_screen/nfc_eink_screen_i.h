#include "nfc_eink_screen.h"
#include "nfc_eink_tag.h"

typedef struct {
    uint16_t width;
    uint16_t height;
    uint8_t data_block_size;
    uint8_t screen_type;
    NfcEinkManufacturer screen_manufacturer;
    const char* name;
} NfcEinkScreenDescriptor; //TODO:NfcEinkScreenBase

typedef struct {
    uint8_t* image_data;
    uint16_t image_size;
    uint16_t received_data;
    NfcEinkScreenDescriptor base;
} NfcEinkScreenData;

typedef void NfcEinkScreenSpecificContext;

typedef struct {
    NfcDevice* nfc_device;
    uint16_t block_total;
    uint16_t block_current;
    NfcEinkScreenSpecificContext* screen_context;
} NfcEinkScreenDevice;

typedef NfcEinkScreenDevice* (*EinkScreenAllocCallback)();
typedef void (*EinkScreenFreeCallback)(NfcEinkScreenDevice* instance);
typedef void (*EinkScreenInitCallback)(NfcEinkScreenData* data, NfcEinkType type);

typedef struct {
    EinkScreenAllocCallback alloc;
    EinkScreenFreeCallback free;
    EinkScreenInitCallback init;
    NfcGenericCallback listener_callback;
    NfcGenericCallback poller_callback;
} NfcEinkScreenHandlers;

struct NfcEinkScreen {
    NfcEinkScreenData* data;
    NfcEinkScreenDevice* device;
    BitBuffer* tx_buf;
    BitBuffer* rx_buf;
    const NfcEinkScreenHandlers* handlers;
    NfcEinkScreenEventCallback event_callback;
    NfcEinkScreenEventContext event_context;

    bool was_update; ///TODO: Candidates to move
    uint8_t update_cnt; //to protocol specific instance
    uint8_t response_cnt;
};

void nfc_eink_screen_vendor_callback(NfcEinkScreen* instance, NfcEinkScreenEventType type);

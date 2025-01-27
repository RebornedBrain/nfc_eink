#pragma once

#include <furi.h>
#include <furi_hal.h>

#include <applications/services/dolphin/dolphin.h>
#include <applications/services/dolphin/helpers/dolphin_deed.h>

#include <assets_icons.h>

#include <../applications/services/notification/notification_messages.h>

#include <../applications/services/gui/gui.h>
#include <../applications/services/gui/view.h>
#include <../applications/services/gui/view_dispatcher.h>
#include <../applications/services/gui/scene_manager.h>
#include <../applications/services/gui/modules/submenu.h>
#include <../applications/services/gui/modules/empty_screen.h>
#include <../applications/services/gui/modules/dialog_ex.h>
#include <../applications/services/gui/modules/popup.h>
#include <../applications/services/gui/modules/loading.h>
#include <../applications/services/gui/modules/text_input.h>
#include <../applications/services/gui/modules/byte_input.h>
#include <../applications/services/gui/modules/text_box.h>
#include <../applications/services/gui/modules/widget.h>
#include <../applications/services/gui/modules/variable_item_list.h>

#include <../applications/services/dialogs/dialogs.h>
#include <../applications/services/storage/storage.h>
//#include <application/services/dolphin/helpers/dolphin_deed.h>

/* #include <lib/nfc/nfc.h>
#include <lib/nfc/nfc_common.h>
#include <lib/nfc/nfc_device.h>
#include <lib/nfc/nfc_listener.h>
#include <lib/nfc/protocols/iso14443_3a/iso14443_3a.h> */
#include <nfc/nfc_listener.h>
#include <nfc/nfc_poller.h>
#include <toolbox/saved_struct.h>

#include "nfc_eink_screen/nfc_eink_screen.h"
#include "scenes/scenes.h"
#include "nfc_eink_tag.h"

#include "views/eink_progress.h"
//#define TAG "NfcEink"

#define NFC_EINK_NAME_SIZE       22
#define NFC_EINK_APP_EXTENSION   ".eink"
#define NFC_EINK_APP_FOLDER_NAME "nfc_eink"

#define NFC_EINK_APP_FOLDER EXT_PATH(NFC_EINK_APP_FOLDER_NAME)

///TODO: remove all this shit below to _i.h file

typedef enum {
    NfcEinkAppCustomEventReserved = 100,

    NfcEinkAppCustomEventTextInputDone,
    NfcEinkAppCustomEventTimerExpired, ///TODO: remove this
    NfcEinkAppCustomEventBlockProcessed,
    NfcEinkAppCustomEventUpdating,
    NfcEinkAppCustomEventProcessFinish,
    NfcEinkAppCustomEventTargetDetected,
    NfcEinkAppCustomEventTargetLost,
    NfcEinkAppCustomEventUnknownError,
} NfcEinkAppCustomEvents;

typedef enum {
    NfcEinkViewMenu,
    NfcEinkViewDialogEx,
    NfcEinkViewPopup,
    NfcEinkViewLoading,
    NfcEinkViewTextInput,
    NfcEinkViewByteInput,
    NfcEinkViewTextBox,
    NfcEinkViewWidget,
    NfcEinkViewVarItemList,
    NfcEinkViewProgress,
    NfcEinkViewEmptyScreen,
} NfcEinkView;

typedef enum {
    NfcEinkWriteModeStrict,
    NfcEinkWriteModeResolution,
    NfcEinkWriteModeManufacturer,
    NfcEinkWriteModeFree
} NfcEinkWriteMode;

typedef struct {
    NfcEinkWriteMode write_mode;
    bool invert_image;
} NfcEinkSettings;

typedef struct {
    Gui* gui;
    DialogsApp* dialogs;
    NotificationApp* notifications;

    ViewDispatcher* view_dispatcher;
    SceneManager* scene_manager;

    // Common Views
    Submenu* submenu;
    DialogEx* dialog_ex;
    Popup* popup;
    Loading* loading;
    TextInput* text_input;
    ByteInput* byte_input;
    TextBox* text_box;
    Widget* widget;
    EmptyScreen* empty_screen;
    VariableItemList* var_item_list;
    EinkProgress* eink_progress;
    View* view_image;

    Nfc* nfc;

    NfcListener* listener;
    NfcPoller* poller;
    NfcEinkScreen* screen;
    const NfcEinkScreenInfo* info_temp;
    EinkScreenInfoArray_t arr;
    NfcEinkSettings settings;

    bool screen_loaded;
    BitBuffer* tx_buf;
    char text_store[50 + 1];
    FuriString* file_path;
    FuriString* file_name;
    FuriTimer* timer;
} NfcEinkApp;

bool nfc_eink_load_from_file_select(NfcEinkApp* instance);
void nfc_eink_blink_emulate_start(NfcEinkApp* app);
void nfc_eink_blink_write_start(NfcEinkApp* app);
void nfc_eink_blink_stop(NfcEinkApp* app);
void nfc_eink_save_settings(NfcEinkApp* instance);

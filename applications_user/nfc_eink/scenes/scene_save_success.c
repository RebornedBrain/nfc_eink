#include "../nfc_eink_app.h"

void nfc_eink_scene_save_success_popup_callback(void* context) {
    NfcEinkApp* nfc = context;
    view_dispatcher_send_custom_event(nfc->view_dispatcher, NfcEinkCustomEventTimerExpired);
}

void nfc_eink_scene_save_success_on_enter(void* context) {
    NfcEinkApp* instance = context;

    // Setup view
    Popup* popup = instance->popup;
    popup_set_icon(popup, 36, 5, &I_DolphinSaved_92x58);
    popup_set_header(popup, "Saved", 15, 19, AlignLeft, AlignBottom);
    popup_set_timeout(popup, 1500);
    popup_set_context(popup, instance);
    popup_set_callback(popup, nfc_eink_scene_save_success_popup_callback);
    popup_enable_timeout(popup);
    view_dispatcher_switch_to_view(instance->view_dispatcher, NfcEinkViewPopup);
}

bool nfc_eink_scene_save_success_on_event(void* context, SceneManagerEvent event) {
    NfcEinkApp* instance = context;

    bool consumed = false;
    if(event.type == SceneManagerEventTypeCustom) {
        if(event.event == NfcEinkCustomEventTimerExpired) {
            consumed = scene_manager_search_and_switch_to_previous_scene(
                instance->scene_manager, NfcEinkAppSceneStart);
        }
    }

    return consumed;
}

void nfc_eink_scene_save_success_on_exit(void* context) {
    NfcEinkApp* nfc = context;

    // Clear view
    popup_reset(nfc->popup);
}

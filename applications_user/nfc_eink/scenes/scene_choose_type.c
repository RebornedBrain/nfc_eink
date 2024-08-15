#include "../nfc_eink_app.h"

static void nfc_eink_scene_choose_type_submenu_callback(void* context, uint32_t index) {
    NfcEinkApp* instance = context;
    view_dispatcher_send_custom_event(instance->view_dispatcher, index);
}

void nfc_eink_scene_choose_type_on_enter(void* context) {
    NfcEinkApp* instance = context;
    Submenu* submenu = instance->submenu;

    submenu_set_header(submenu, "Choose Manufacturer Type");
    for(uint8_t type = 0; type < NfcEinkManufacturerNum /* NfcEinkTypeNum */; type++) {
        submenu_add_item(
            submenu,
            nfc_eink_screen_get_manufacturer_name(type),
            type,
            nfc_eink_scene_choose_type_submenu_callback,
            instance);
    }

    view_dispatcher_switch_to_view(instance->view_dispatcher, NfcEinkViewMenu);
}

bool nfc_eink_scene_choose_type_on_event(void* context, SceneManagerEvent event) {
    NfcEinkApp* instance = context;
    SceneManager* scene_manager = instance->scene_manager;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        const NfcEinkManufacturer screen_manufacturer = event.event;
        instance->screen =
            nfc_eink_screen_alloc(screen_manufacturer /* NfcEinkManufacturerWaveshare */);
        consumed = true;
        scene_manager_next_scene(scene_manager, NfcEinkAppSceneEmulate);
    }

    return consumed;
}

void nfc_eink_scene_choose_type_on_exit(void* context) {
    NfcEinkApp* instance = context;
    submenu_reset(instance->submenu);
}

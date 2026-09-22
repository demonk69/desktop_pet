#include <assert.h>
#include <stdio.h>

#include "config/pet_config.h"
#include "pet/pet_app.h"
#include "ui/pet_scene.h"

static pet_event_t event(pet_event_type_t type)
{
    return (pet_event_t){ .type = type };
}

static void init_idle_app(pet_app_t *app, pet_config_t *config)
{
    pet_config_set_development_defaults(config);
    config->app.behavior.auto_blink_min_interval_ms = 60000U;
    config->app.behavior.auto_blink_max_interval_ms = 60000U;
    config->app.behavior.idle_look_inactivity_ms = 60000U;
    config->app.behavior.idle_look_min_interval_ms = 60000U;
    config->app.behavior.idle_look_max_interval_ms = 60000U;
    assert(pet_app_init(app, &config->app) == PET_STATUS_OK);
    pet_app_update(app, config->app.core.boot_duration_ms);
    assert(pet_app_snapshot(app).state == PET_STATE_IDLE);
}

int main(void)
{
    pet_scene_manager_t manager;
    pet_ui_snapshot_t ui;
    pet_event_t input;
    pet_config_t config;
    pet_app_t app;
    pet_app_snapshot_t snapshot;

    assert(pet_scene_manager_init(&manager) == PET_STATUS_OK);
    ui = pet_scene_manager_get_state(&manager);
    assert(ui.current == PET_UI_SCENE_HOME);
    assert(ui.selected == PET_UI_SCENE_HOME);
    assert(!ui.entered);

    input = event(PET_EVENT_BUTTON);
    assert(!pet_scene_manager_handle_event(&manager, &input));
    ui = pet_scene_manager_get_state(&manager);
    assert(ui.current == PET_UI_SCENE_HOME);
    assert(ui.selected == PET_UI_SCENE_HOME);
    assert(!ui.entered);

    input = event(PET_EVENT_NAV_NEXT);
    assert(pet_scene_manager_handle_event(&manager, &input));
    ui = pet_scene_manager_get_state(&manager);
    assert(ui.current == PET_UI_SCENE_HOME);
    assert(ui.selected == PET_UI_SCENE_CLOCK);

    input = event(PET_EVENT_NAV_PREV);
    assert(pet_scene_manager_handle_event(&manager, &input));
    ui = pet_scene_manager_get_state(&manager);
    assert(ui.current == PET_UI_SCENE_HOME);
    assert(ui.selected == PET_UI_SCENE_HOME);

    input = event(PET_EVENT_NAV_PREV);
    assert(pet_scene_manager_handle_event(&manager, &input));
    ui = pet_scene_manager_get_state(&manager);
    assert(ui.selected == PET_UI_SCENE_SETTINGS);

    input = event(PET_EVENT_NAV_NEXT);
    assert(pet_scene_manager_handle_event(&manager, &input));
    input = event(PET_EVENT_NAV_NEXT);
    assert(pet_scene_manager_handle_event(&manager, &input));
    ui = pet_scene_manager_get_state(&manager);
    assert(ui.selected == PET_UI_SCENE_CLOCK);

    input = event(PET_EVENT_BUTTON);
    assert(pet_scene_manager_handle_event(&manager, &input));
    ui = pet_scene_manager_get_state(&manager);
    assert(ui.current == PET_UI_SCENE_CLOCK);
    assert(ui.selected == PET_UI_SCENE_CLOCK);
    assert(ui.entered);

    input = event(PET_EVENT_NAV_NEXT);
    assert(pet_scene_manager_handle_event(&manager, &input));
    ui = pet_scene_manager_get_state(&manager);
    assert(ui.current == PET_UI_SCENE_CLOCK);
    assert(ui.selected == PET_UI_SCENE_CLOCK);
    assert(ui.entered);

    input = event(PET_EVENT_NAV_PREV);
    assert(pet_scene_manager_handle_event(&manager, &input));
    ui = pet_scene_manager_get_state(&manager);
    assert(ui.current == PET_UI_SCENE_CLOCK);
    assert(ui.selected == PET_UI_SCENE_CLOCK);
    assert(ui.entered);

    input = event(PET_EVENT_BUTTON);
    assert(pet_scene_manager_handle_event(&manager, &input));
    ui = pet_scene_manager_get_state(&manager);
    assert(ui.current == PET_UI_SCENE_HOME);
    assert(ui.selected == PET_UI_SCENE_HOME);
    assert(!ui.entered);

    input = event(PET_EVENT_NAV_NEXT);
    assert(pet_scene_manager_handle_event(&manager, &input));
    input = event(PET_EVENT_NAV_NEXT);
    assert(pet_scene_manager_handle_event(&manager, &input));
    input = event(PET_EVENT_BUTTON);
    assert(pet_scene_manager_handle_event(&manager, &input));
    ui = pet_scene_manager_get_state(&manager);
    assert(ui.current == PET_UI_SCENE_STATUS);
    input = event(PET_EVENT_BUTTON);
    assert(pet_scene_manager_handle_event(&manager, &input));
    assert(pet_scene_manager_get_state(&manager).current == PET_UI_SCENE_HOME);

    input = event(PET_EVENT_NAV_PREV);
    assert(pet_scene_manager_handle_event(&manager, &input));
    input = event(PET_EVENT_BUTTON);
    assert(pet_scene_manager_handle_event(&manager, &input));
    ui = pet_scene_manager_get_state(&manager);
    assert(ui.current == PET_UI_SCENE_SETTINGS);
    input = event(PET_EVENT_BUTTON);
    assert(pet_scene_manager_handle_event(&manager, &input));
    assert(pet_scene_manager_get_state(&manager).current == PET_UI_SCENE_HOME);

    init_idle_app(&app, &config);
    input = event(PET_EVENT_NAV_NEXT);
    assert(pet_app_post_event(&app, &input));
    pet_app_update(&app, 0U);
    snapshot = pet_app_snapshot(&app);
    assert(snapshot.state == PET_STATE_IDLE);
    assert(snapshot.ui.current == PET_UI_SCENE_HOME);
    assert(snapshot.ui.selected == PET_UI_SCENE_CLOCK);

    input = event(PET_EVENT_BUTTON);
    assert(pet_app_post_event(&app, &input));
    pet_app_update(&app, 0U);
    snapshot = pet_app_snapshot(&app);
    assert(snapshot.state == PET_STATE_IDLE);
    assert(snapshot.ui.current == PET_UI_SCENE_CLOCK);
    assert(snapshot.ui.entered);

    input = event(PET_EVENT_BUTTON);
    assert(pet_app_post_event(&app, &input));
    pet_app_update(&app, 0U);
    snapshot = pet_app_snapshot(&app);
    assert(snapshot.state == PET_STATE_IDLE);
    assert(snapshot.ui.current == PET_UI_SCENE_HOME);
    assert(snapshot.ui.selected == PET_UI_SCENE_HOME);
    assert(!snapshot.ui.entered);

    init_idle_app(&app, &config);
    input = event(PET_EVENT_BUTTON);
    assert(pet_app_post_event(&app, &input));
    pet_app_update(&app, 0U);
    snapshot = pet_app_snapshot(&app);
    assert(snapshot.state == PET_STATE_HAPPY);
    assert(snapshot.animation == PET_ANIM_HAPPY);
    assert(snapshot.ui.current == PET_UI_SCENE_HOME);
    assert(snapshot.ui.selected == PET_UI_SCENE_HOME);
    assert(!snapshot.ui.entered);

    (void)printf("test_scene_manager: ok\n");
    return 0;
}

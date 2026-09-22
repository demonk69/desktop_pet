#include <assert.h>
#include <stddef.h>
#include <stdio.h>

#include "animation/pet_assets.h"
#include "config/pet_config.h"
#include "pet/pet_app.h"

static void post_event(pet_app_t *app, pet_event_type_t type)
{
    pet_event_t event = { .type = type, .timestamp_ms = app->now_ms };
    assert(pet_app_post_event(app, &event));
}

static void configure_quiet_behavior(pet_config_t *config)
{
    config->app.behavior.auto_blink_min_interval_ms = 60000U;
    config->app.behavior.auto_blink_max_interval_ms = 60000U;
    config->app.behavior.idle_look_inactivity_ms = 60000U;
    config->app.behavior.idle_look_min_interval_ms = 60000U;
    config->app.behavior.idle_look_max_interval_ms = 60000U;
}

static void init_idle_app(pet_app_t *app, pet_config_t *config)
{
    pet_config_set_development_defaults(config);
    configure_quiet_behavior(config);
    assert(pet_app_init(app, &config->app) == PET_STATUS_OK);
    pet_app_update(app, config->app.core.boot_duration_ms);
    assert(pet_app_snapshot(app).state == PET_STATE_IDLE);
}

static void assert_sleeps_at_configured_inactivity_timeout(pet_app_t *app)
{
    uint32_t elapsed_ms;
    uint32_t remaining_ms;

    assert(app != NULL);
    elapsed_ms = app->now_ms - app->last_user_activity_ms;
    assert(elapsed_ms < app->core_config.inactivity_sleep_ms);
    remaining_ms = app->core_config.inactivity_sleep_ms - elapsed_ms;
    assert(remaining_ms > 1U);
    pet_app_update(app, remaining_ms - 1U);
    assert(pet_app_snapshot(app).state != PET_STATE_SLEEP);
    pet_app_update(app, 1U);
    assert(pet_app_snapshot(app).state == PET_STATE_SLEEP);
}

int main(void)
{
    pet_config_t config;
    pet_config_t nav_config;
    pet_config_t sleep_config;
    pet_config_t inactive_config;
    pet_config_t reset_config;
    pet_config_t burst_config;
    pet_config_t interact_config;
    pet_app_t app;
    pet_app_t nav_app;
    pet_app_t sleep_app;
    pet_app_t inactive_app;
    pet_app_t reset_app;
    pet_app_t burst_app;
    pet_app_t interact_app;
    pet_app_snapshot_t snapshot;
    pet_event_t event;
    size_t index;

    pet_config_set_development_defaults(&config);
    config.app.behavior.auto_blink_min_interval_ms = 3000U;
    config.app.behavior.auto_blink_max_interval_ms = 3000U;
    config.app.behavior.idle_look_inactivity_ms = 60000U;
    config.app.behavior.idle_look_min_interval_ms = 60000U;
    config.app.behavior.idle_look_max_interval_ms = 60000U;
    assert(pet_app_init(&app, &config.app) == PET_STATUS_OK);
    snapshot = pet_app_snapshot(&app);
    assert(snapshot.state == PET_STATE_BOOT);
    assert(snapshot.animation == PET_ANIM_BOOT);

    pet_app_update(&app, config.app.core.boot_duration_ms);
    snapshot = pet_app_snapshot(&app);
    assert(snapshot.state == PET_STATE_IDLE);
    assert(snapshot.animation == PET_ANIM_IDLE);
    assert(snapshot.frame->asset_id == PET_ASSET_TEST_CHARACTER);
    pet_app_update(&app, 700U);
    snapshot = pet_app_snapshot(&app);
    assert(snapshot.state == PET_STATE_IDLE);
    assert(snapshot.frame->asset_id == PET_ASSET_TEST_CHARACTER_IDLE_1);
    pet_app_update(&app, 700U);
    snapshot = pet_app_snapshot(&app);
    assert(snapshot.state == PET_STATE_IDLE);
    assert(snapshot.frame->asset_id == PET_ASSET_TEST_CHARACTER);

    assert(config.app.behavior.auto_blink_min_interval_ms > 1400U);
    pet_app_update(&app,
                   config.app.behavior.auto_blink_min_interval_ms - 1400U);
    snapshot = pet_app_snapshot(&app);
    assert(snapshot.state == PET_STATE_BLINK);
    assert(snapshot.frame->asset_id == PET_ASSET_TEST_CHARACTER_BLINK_HALF);
    pet_app_update(&app, 80U);
    snapshot = pet_app_snapshot(&app);
    assert(snapshot.state == PET_STATE_BLINK);
    assert(snapshot.frame->asset_id == PET_ASSET_TEST_CHARACTER_BLINK_CLOSED);
    pet_app_update(&app, 100U);
    assert(pet_app_snapshot(&app).frame->asset_id ==
           PET_ASSET_TEST_CHARACTER_BLINK_HALF);
    pet_app_update(&app, 80U);
    assert(pet_app_snapshot(&app).frame->asset_id == PET_ASSET_TEST_CHARACTER);
    pet_app_update(&app, 80U);
    snapshot = pet_app_snapshot(&app);
    assert(snapshot.state == PET_STATE_IDLE);
    assert(snapshot.animation == PET_ANIM_IDLE);
    assert(snapshot.frame->asset_id == PET_ASSET_TEST_CHARACTER);

    post_event(&app, PET_EVENT_SLEEP);
    pet_app_update(&app, 0U);
    assert(pet_app_snapshot(&app).state == PET_STATE_SLEEP);
    post_event(&app, PET_EVENT_WAKE);
    pet_app_update(&app, 0U);
    assert(pet_app_snapshot(&app).state == PET_STATE_IDLE);

    event = (pet_event_t){ .type = PET_EVENT_MESSAGE,
                           .timestamp_ms = app.now_ms,
                           .data.message_id = 1U };
    assert(pet_app_post_event(&app, &event));
    pet_app_update(&app, 0U);
    snapshot = pet_app_snapshot(&app);
    assert(snapshot.state == PET_STATE_HAPPY);
    assert(snapshot.frame->asset_id == PET_ASSET_TEST_CHARACTER_HAPPY_0);
    pet_app_update(&app, 180U);
    snapshot = pet_app_snapshot(&app);
    assert(snapshot.state == PET_STATE_HAPPY);
    assert(snapshot.frame->asset_id == PET_ASSET_TEST_CHARACTER_HAPPY_1);

    event = (pet_event_t){ .type = PET_EVENT_LOOK,
                           .timestamp_ms = app.now_ms,
                           .data.look_direction = PET_LOOK_LEFT };
    assert(pet_app_post_event(&app, &event));
    pet_app_update(&app, 0U);
    assert(pet_app_snapshot(&app).state == PET_STATE_LOOK_LEFT);

    /* A completion queued for an old animation must not cancel newer input. */
    event = (pet_event_t){ .type = PET_EVENT_HAPPY, .timestamp_ms = app.now_ms };
    assert(pet_app_post_event(&app, &event));
    event = (pet_event_t){ .type = PET_EVENT_ANIMATION_DONE,
                           .timestamp_ms = app.now_ms,
                           .data.code = PET_ANIM_LOOK_LEFT };
    assert(pet_app_post_event(&app, &event));
    pet_app_update(&app, 0U);
    assert(pet_app_snapshot(&app).state == PET_STATE_HAPPY);

    init_idle_app(&nav_app, &nav_config);
    assert(pet_app_snapshot(&nav_app).ui.current == PET_UI_SCENE_HOME);
    assert(pet_app_snapshot(&nav_app).ui.selected == PET_UI_SCENE_HOME);
    post_event(&nav_app, PET_EVENT_NAV_NEXT);
    pet_app_update(&nav_app, 0U);
    assert(pet_app_snapshot(&nav_app).state == PET_STATE_IDLE);
    assert(pet_app_snapshot(&nav_app).ui.selected == PET_UI_SCENE_CLOCK);
    post_event(&nav_app, PET_EVENT_NAV_PREV);
    pet_app_update(&nav_app, 0U);
    assert(pet_app_snapshot(&nav_app).state == PET_STATE_IDLE);
    assert(pet_app_snapshot(&nav_app).ui.selected == PET_UI_SCENE_HOME);
    post_event(&nav_app, PET_EVENT_BUTTON);
    pet_app_update(&nav_app, 0U);
    snapshot = pet_app_snapshot(&nav_app);
    assert(snapshot.state == PET_STATE_HAPPY);
    assert(snapshot.animation == PET_ANIM_HAPPY);
    assert(snapshot.frame->asset_id == PET_ASSET_TEST_CHARACTER_HAPPY_0);

    init_idle_app(&nav_app, &nav_config);
    post_event(&nav_app, PET_EVENT_NAV_NEXT);
    pet_app_update(&nav_app, 0U);
    assert(pet_app_snapshot(&nav_app).ui.selected == PET_UI_SCENE_CLOCK);
    post_event(&nav_app, PET_EVENT_BUTTON);
    pet_app_update(&nav_app, 0U);
    assert(pet_app_snapshot(&nav_app).state == PET_STATE_IDLE);
    assert(pet_app_snapshot(&nav_app).ui.current == PET_UI_SCENE_CLOCK);
    assert(pet_app_snapshot(&nav_app).ui.entered);
    post_event(&nav_app, PET_EVENT_BUTTON);
    pet_app_update(&nav_app, 0U);
    assert(pet_app_snapshot(&nav_app).ui.current == PET_UI_SCENE_HOME);
    assert(!pet_app_snapshot(&nav_app).ui.entered);

    init_idle_app(&sleep_app, &sleep_config);
    post_event(&sleep_app, PET_EVENT_SLEEP);
    pet_app_update(&sleep_app, 0U);
    assert(pet_app_snapshot(&sleep_app).state == PET_STATE_SLEEP);
    post_event(&sleep_app, PET_EVENT_NAV_NEXT);
    pet_app_update(&sleep_app, 0U);
    assert(pet_app_snapshot(&sleep_app).state == PET_STATE_IDLE);
    post_event(&sleep_app, PET_EVENT_SLEEP);
    pet_app_update(&sleep_app, 0U);
    assert(pet_app_snapshot(&sleep_app).state == PET_STATE_SLEEP);
    post_event(&sleep_app, PET_EVENT_BUTTON);
    pet_app_update(&sleep_app, 0U);
    assert(pet_app_snapshot(&sleep_app).state == PET_STATE_IDLE);

    init_idle_app(&inactive_app, &inactive_config);
    assert_sleeps_at_configured_inactivity_timeout(&inactive_app);

    init_idle_app(&reset_app, &reset_config);
    pet_app_update(&reset_app, reset_app.core_config.inactivity_sleep_ms / 2U);
    assert(pet_app_snapshot(&reset_app).state != PET_STATE_SLEEP);
    post_event(&reset_app, PET_EVENT_NAV_NEXT);
    pet_app_update(&reset_app, 0U);
    assert(pet_app_snapshot(&reset_app).state == PET_STATE_IDLE);
    assert_sleeps_at_configured_inactivity_timeout(&reset_app);

    init_idle_app(&burst_app, &burst_config);
    for (index = 0U; index < 8U; index++) {
        post_event(&burst_app, PET_EVENT_NAV_NEXT);
    }
    pet_app_update(&burst_app, 0U);
    assert(pet_app_snapshot(&burst_app).state == PET_STATE_IDLE);
    assert(pet_app_snapshot(&burst_app).ui.selected == PET_UI_SCENE_HOME);

    init_idle_app(&interact_app, &interact_config);
    post_event(&interact_app, PET_EVENT_BUTTON);
    pet_app_update(&interact_app, 0U);
    assert(pet_app_snapshot(&interact_app).state == PET_STATE_HAPPY);
    assert(pet_app_snapshot(&interact_app).ui.current == PET_UI_SCENE_HOME);
    post_event(&interact_app, PET_EVENT_SLEEP);
    pet_app_update(&interact_app, 0U);
    assert(pet_app_snapshot(&interact_app).state == PET_STATE_SLEEP);
    post_event(&interact_app, PET_EVENT_BUTTON);
    pet_app_update(&interact_app, 0U);
    assert(pet_app_snapshot(&interact_app).state == PET_STATE_IDLE);
    assert_sleeps_at_configured_inactivity_timeout(&interact_app);

    (void)printf("test_app_integration: ok\n");
    return 0;
}

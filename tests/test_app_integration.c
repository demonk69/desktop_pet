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

static void init_idle_app(pet_app_t *app, pet_config_t *config)
{
    pet_config_set_development_defaults(config);
    config->app.core.idle_action_interval_ms = 60000U;
    assert(pet_app_init(app, &config->app) == PET_STATUS_OK);
    pet_app_update(app, config->app.core.boot_duration_ms);
    assert(pet_app_snapshot(app).state == PET_STATE_IDLE);
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
    assert(pet_app_init(&app, &config.app) == PET_STATUS_OK);
    snapshot = pet_app_snapshot(&app);
    assert(snapshot.state == PET_STATE_BOOT);
    assert(snapshot.animation == PET_ANIM_BOOT);

    pet_app_update(&app, config.app.core.boot_duration_ms);
    snapshot = pet_app_snapshot(&app);
    assert(snapshot.state == PET_STATE_IDLE);
    assert(snapshot.animation == PET_ANIM_IDLE);

    pet_app_update(&app, config.app.core.idle_action_interval_ms);
    snapshot = pet_app_snapshot(&app);
    assert(snapshot.state == PET_STATE_BLINK);
    assert(snapshot.frame->asset_id == PET_ASSET_BLINK_HALF);
    pet_app_update(&app, 80U);
    snapshot = pet_app_snapshot(&app);
    assert(snapshot.state == PET_STATE_BLINK);
    assert(snapshot.frame->asset_id == PET_ASSET_BLINK_CLOSED);
    pet_app_update(&app, 100U);
    assert(pet_app_snapshot(&app).frame->asset_id == PET_ASSET_BLINK_HALF);
    pet_app_update(&app, 80U);
    assert(pet_app_snapshot(&app).frame->asset_id == PET_ASSET_IDLE);
    pet_app_update(&app, 80U);
    snapshot = pet_app_snapshot(&app);
    assert(snapshot.state == PET_STATE_IDLE);
    assert(snapshot.animation == PET_ANIM_IDLE);

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
    assert(pet_app_snapshot(&app).state == PET_STATE_HAPPY);

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
    post_event(&nav_app, PET_EVENT_NAV_NEXT);
    pet_app_update(&nav_app, 0U);
    assert(pet_app_snapshot(&nav_app).state == PET_STATE_LOOK_RIGHT);
    pet_app_update(&nav_app, 620U);
    assert(pet_app_snapshot(&nav_app).state == PET_STATE_IDLE);
    post_event(&nav_app, PET_EVENT_NAV_PREV);
    pet_app_update(&nav_app, 0U);
    assert(pet_app_snapshot(&nav_app).state == PET_STATE_LOOK_LEFT);
    pet_app_update(&nav_app, 620U);
    assert(pet_app_snapshot(&nav_app).state == PET_STATE_IDLE);

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
    post_event(&sleep_app, PET_EVENT_NAV_PREV);
    pet_app_update(&sleep_app, 0U);
    assert(pet_app_snapshot(&sleep_app).state == PET_STATE_IDLE);

    init_idle_app(&inactive_app, &inactive_config);
    pet_app_update(&inactive_app, 14399U);
    assert(pet_app_snapshot(&inactive_app).state != PET_STATE_SLEEP);
    pet_app_update(&inactive_app, 1U);
    assert(pet_app_snapshot(&inactive_app).state == PET_STATE_SLEEP);

    init_idle_app(&reset_app, &reset_config);
    pet_app_update(&reset_app, 10000U);
    assert(pet_app_snapshot(&reset_app).state != PET_STATE_SLEEP);
    post_event(&reset_app, PET_EVENT_NAV_NEXT);
    pet_app_update(&reset_app, 0U);
    assert(pet_app_snapshot(&reset_app).state == PET_STATE_LOOK_RIGHT);
    pet_app_update(&reset_app, 620U);
    assert(pet_app_snapshot(&reset_app).state == PET_STATE_IDLE);
    pet_app_update(&reset_app, 14379U);
    assert(pet_app_snapshot(&reset_app).state != PET_STATE_SLEEP);
    pet_app_update(&reset_app, 1U);
    assert(pet_app_snapshot(&reset_app).state == PET_STATE_SLEEP);

    init_idle_app(&burst_app, &burst_config);
    for (index = 0U; index < 8U; index++) {
        post_event(&burst_app, PET_EVENT_NAV_NEXT);
    }
    pet_app_update(&burst_app, 0U);
    assert(pet_app_snapshot(&burst_app).state == PET_STATE_LOOK_RIGHT);

    init_idle_app(&interact_app, &interact_config);
    post_event(&interact_app, PET_EVENT_BUTTON);
    pet_app_update(&interact_app, 0U);
    assert(pet_app_snapshot(&interact_app).state == PET_STATE_HAPPY);
    pet_app_update(&interact_app, 720U);
    assert(pet_app_snapshot(&interact_app).state == PET_STATE_IDLE);
    post_event(&interact_app, PET_EVENT_SLEEP);
    pet_app_update(&interact_app, 0U);
    assert(pet_app_snapshot(&interact_app).state == PET_STATE_SLEEP);
    post_event(&interact_app, PET_EVENT_BUTTON);
    pet_app_update(&interact_app, 0U);
    assert(pet_app_snapshot(&interact_app).state == PET_STATE_IDLE);
    pet_app_update(&interact_app, 14999U);
    assert(pet_app_snapshot(&interact_app).state != PET_STATE_SLEEP);
    pet_app_update(&interact_app, 1U);
    assert(pet_app_snapshot(&interact_app).state == PET_STATE_SLEEP);

    (void)printf("test_app_integration: ok\n");
    return 0;
}

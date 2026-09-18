#include <assert.h>
#include <stdio.h>

#include "animation/pet_assets.h"
#include "config/pet_config.h"
#include "pet/pet_app.h"

static void post_event(pet_app_t *app, pet_event_type_t type)
{
    pet_event_t event = { .type = type, .timestamp_ms = app->now_ms };
    assert(pet_app_post_event(app, &event));
}

int main(void)
{
    pet_config_t config;
    pet_app_t app;
    pet_app_snapshot_t snapshot;
    pet_event_t event;

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

    (void)printf("test_app_integration: ok\n");
    return 0;
}

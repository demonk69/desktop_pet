#include <assert.h>
#include <stdio.h>

#include "animation/pet_assets.h"
#include "behavior/pet_behavior.h"
#include "config/pet_config.h"
#include "pet/pet_app.h"

static void quiet_look(pet_behavior_config_t *behavior)
{
    behavior->idle_look_inactivity_ms = 60000U;
    behavior->idle_look_min_interval_ms = 60000U;
    behavior->idle_look_max_interval_ms = 60000U;
}

static void quiet_blink(pet_behavior_config_t *behavior)
{
    behavior->auto_blink_min_interval_ms = 60000U;
    behavior->auto_blink_max_interval_ms = 60000U;
}

static void init_idle_app(pet_app_t *app, pet_config_t *config)
{
    assert(pet_app_init(app, &config->app) == PET_STATUS_OK);
    pet_app_update(app, config->app.core.boot_duration_ms);
    assert(pet_app_snapshot(app).state == PET_STATE_IDLE);
}

static void assert_direct_auto_blink_event(void)
{
    pet_behavior_manager_t manager;
    pet_behavior_config_t config = { .auto_blink_min_interval_ms = 3000U,
                                     .auto_blink_max_interval_ms = 3000U,
                                     .idle_look_inactivity_ms = 60000U,
                                     .idle_look_min_interval_ms = 60000U,
                                     .idle_look_max_interval_ms = 60000U,
                                     .random_seed = 1U };
    pet_behavior_context_t context = { .now_ms = 0U,
                                       .last_user_activity_ms = 0U,
                                       .state = PET_STATE_BOOT };
    pet_event_t event;

    assert(pet_behavior_manager_init(&manager, &config) == PET_STATUS_OK);
    assert(!pet_behavior_manager_update(&manager, &context, &event));
    context.now_ms = 600U;
    context.state = PET_STATE_IDLE;
    assert(!pet_behavior_manager_update(&manager, &context, &event));
    context.now_ms = 3599U;
    assert(!pet_behavior_manager_update(&manager, &context, &event));
    context.now_ms = 3600U;
    assert(pet_behavior_manager_update(&manager, &context, &event));
    assert(event.type == PET_EVENT_BLINK);
    assert(event.timestamp_ms == 3600U);
    assert((event.flags & PET_EVENT_FLAG_AUTONOMOUS) != 0U);
}

static void assert_app_auto_blink_with_fake_timer(void)
{
    pet_config_t config;
    pet_app_t app;
    pet_app_snapshot_t snapshot;

    pet_config_set_development_defaults(&config);
    config.app.core.inactivity_sleep_ms = 60000U;
    config.app.behavior.auto_blink_min_interval_ms = 3000U;
    config.app.behavior.auto_blink_max_interval_ms = 3000U;
    quiet_look(&config.app.behavior);

    init_idle_app(&app, &config);
    pet_app_update(&app, 2999U);
    assert(pet_app_snapshot(&app).state == PET_STATE_IDLE);
    pet_app_update(&app, 1U);
    snapshot = pet_app_snapshot(&app);
    assert(snapshot.state == PET_STATE_BLINK);
    assert(snapshot.animation == PET_ANIM_BLINK);
    assert(snapshot.frame->asset_id == PET_ASSET_TEST_CHARACTER_BLINK_HALF);
    pet_app_update(&app, 340U);
    snapshot = pet_app_snapshot(&app);
    assert(snapshot.state == PET_STATE_IDLE);
    assert(snapshot.animation == PET_ANIM_IDLE);
}

static void assert_idle_look_is_autonomous(void)
{
    pet_config_t config;
    pet_app_t app;
    pet_app_snapshot_t snapshot;

    pet_config_set_development_defaults(&config);
    config.app.core.inactivity_sleep_ms = 60000U;
    quiet_blink(&config.app.behavior);
    config.app.behavior.idle_look_inactivity_ms = 1000U;
    config.app.behavior.idle_look_min_interval_ms = 500U;
    config.app.behavior.idle_look_max_interval_ms = 500U;

    init_idle_app(&app, &config);
    pet_app_update(&app, 899U);
    assert(pet_app_snapshot(&app).state == PET_STATE_IDLE);
    pet_app_update(&app, 1U);
    snapshot = pet_app_snapshot(&app);
    assert(snapshot.state == PET_STATE_LOOK_LEFT ||
           snapshot.state == PET_STATE_LOOK_RIGHT);
    assert(app.last_user_activity_ms == 0U);
}

static void assert_happy_recovers_to_idle(void)
{
    pet_config_t config;
    pet_app_t app;
    pet_event_t event;

    pet_config_set_development_defaults(&config);
    config.app.core.inactivity_sleep_ms = 60000U;
    quiet_blink(&config.app.behavior);
    quiet_look(&config.app.behavior);
    init_idle_app(&app, &config);

    event = (pet_event_t){ .type = PET_EVENT_BUTTON,
                           .timestamp_ms = app.now_ms };
    assert(pet_app_post_event(&app, &event));
    pet_app_update(&app, 0U);
    assert(pet_app_snapshot(&app).state == PET_STATE_HAPPY);
    pet_app_update(&app, 720U);
    assert(pet_app_snapshot(&app).state == PET_STATE_IDLE);
}

int main(void)
{
    assert_direct_auto_blink_event();
    assert_app_auto_blink_with_fake_timer();
    assert_idle_look_is_autonomous();
    assert_happy_recovers_to_idle();
    (void)printf("test_behavior_manager: ok\n");
    return 0;
}

#include "pet/pet_app.h"

static void apply_animation_request(pet_app_t *app)
{
    pet_animation_id_t requested;
    if (pet_core_take_animation_request(&app->core, &requested)) {
        (void)pet_play_animation(&app->animation, requested);
    }
}

static bool is_user_activity_event(const pet_event_t *event)
{
    if (event == NULL) {
        return false;
    }
    if ((event->flags & PET_EVENT_FLAG_AUTONOMOUS) != 0U) {
        return false;
    }
    switch (event->type) {
    case PET_EVENT_BUTTON:
    case PET_EVENT_LOOK:
    case PET_EVENT_NAV_NEXT:
    case PET_EVENT_NAV_PREV:
    case PET_EVENT_WAKE:
        return true;
    default:
        return false;
    }
}

static void process_queued_events(pet_app_t *app)
{
    pet_event_t event;
    while (pet_event_queue_pop(&app->events, &event)) {
        bool core_sleeping = pet_core_state(&app->core) == PET_STATE_SLEEP;
        bool consumed = core_sleeping ? false
                                      : pet_scene_manager_handle_event(&app->scene,
                                                                       &event);
        bool handled = consumed;
        if (!consumed) {
            handled = pet_core_handle_event(&app->core, &app->core_config, &event);
        }
        if (handled && is_user_activity_event(&event)) {
            app->last_user_activity_ms = app->now_ms;
        }
        if (!consumed) {
            apply_animation_request(app);
        }
    }
}

static void post_inactivity_sleep_if_due(pet_app_t *app)
{
    pet_event_t event;
    if (pet_core_state(&app->core) == PET_STATE_SLEEP ||
        app->now_ms - app->last_user_activity_ms < app->core_config.inactivity_sleep_ms) {
        return;
    }
    event = (pet_event_t){ .type = PET_EVENT_SLEEP, .timestamp_ms = app->now_ms };
    if (pet_event_queue_push(&app->events, &event)) {
        process_queued_events(app);
    }
}

static void post_behavior_event_if_due(pet_app_t *app)
{
    pet_event_t event;
    pet_behavior_context_t context;

    context = (pet_behavior_context_t){ .now_ms = app->now_ms,
                                       .last_user_activity_ms =
                                           app->last_user_activity_ms,
                                       .state = pet_core_state(&app->core) };
    if (pet_behavior_manager_update(&app->behavior, &context, &event) &&
        pet_event_queue_push(&app->events, &event)) {
        process_queued_events(app);
    }
}

pet_status_t pet_app_init(pet_app_t *app, const pet_app_config_t *config)
{
    pet_status_t status;

    if (app == NULL || config == NULL || config->animations == NULL) {
        return PET_STATUS_INVALID_ARGUMENT;
    }
    app->core_config = config->core;
    app->now_ms = 0U;
    app->last_user_activity_ms = 0U;
    pet_event_queue_init(&app->events);
    status = pet_core_init(&app->core, &app->core_config);
    if (status != PET_STATUS_OK) {
        return status;
    }
    status = pet_behavior_manager_init(&app->behavior, &config->behavior);
    if (status != PET_STATUS_OK) {
        return status;
    }
    status = pet_scene_manager_init(&app->scene);
    if (status != PET_STATUS_OK) {
        return status;
    }
    status = pet_animation_player_init(&app->animation, config->animations);
    if (status != PET_STATUS_OK) {
        return status;
    }
    apply_animation_request(app);
    return PET_STATUS_OK;
}

bool pet_app_post_event(pet_app_t *app, const pet_event_t *event)
{
    return app != NULL && pet_event_queue_push(&app->events, event);
}

void pet_app_set_time_snapshot(pet_app_t *app,
                               const pet_time_snapshot_t *snapshot)
{
    if (app != NULL) {
        pet_scene_manager_set_time_snapshot(&app->scene, snapshot);
    }
}

void pet_app_update(pet_app_t *app, uint32_t delta_ms)
{
    pet_event_t event;
    pet_animation_id_t completed;

    if (app == NULL) {
        return;
    }
    app->now_ms += delta_ms;
    pet_animation_update(&app->animation, delta_ms);
    if (pet_animation_take_completed(&app->animation, &completed)) {
        event = (pet_event_t){ .type = PET_EVENT_ANIMATION_DONE,
                               .timestamp_ms = app->now_ms,
                               .data.code = (int32_t)completed };
        (void)pet_event_queue_push(&app->events, &event);
    }
    event = (pet_event_t){ .type = PET_EVENT_TIMER,
                           .timestamp_ms = app->now_ms,
                           .data.timer_delta_ms = delta_ms };
    (void)pet_event_queue_push(&app->events, &event);

    process_queued_events(app);
    post_behavior_event_if_due(app);
    post_inactivity_sleep_if_due(app);
}

pet_app_snapshot_t pet_app_snapshot(const pet_app_t *app)
{
    pet_app_snapshot_t snapshot = { PET_STATE_COUNT, PET_ANIM_COUNT, NULL,
                                    { PET_UI_SCENE_COUNT, PET_UI_SCENE_COUNT,
                                      false, { 0 } } };
    if (app != NULL) {
        snapshot.state = pet_core_state(&app->core);
        snapshot.animation = pet_animation_current_id(&app->animation);
        snapshot.frame = pet_animation_current_frame(&app->animation);
        snapshot.ui = pet_scene_manager_get_state(&app->scene);
    }
    return snapshot;
}

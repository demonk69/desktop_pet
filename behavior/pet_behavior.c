#include "behavior/pet_behavior.h"

#include <stdint.h>

static bool behavior_config_valid(const pet_behavior_config_t *config)
{
    return config != NULL && config->auto_blink_min_interval_ms > 0U &&
           config->auto_blink_min_interval_ms <=
               config->auto_blink_max_interval_ms &&
           config->idle_look_inactivity_ms > 0U &&
           config->idle_look_min_interval_ms > 0U &&
           config->idle_look_min_interval_ms <=
               config->idle_look_max_interval_ms;
}

static uint32_t add_clamped(uint32_t left, uint32_t right)
{
    return UINT32_MAX - left < right ? UINT32_MAX : left + right;
}

static bool time_reached(uint32_t now_ms, uint32_t due_ms)
{
    return (int32_t)(now_ms - due_ms) >= 0;
}

static uint32_t next_random(pet_behavior_manager_t *manager)
{
    manager->random_state = manager->random_state * 1664525U + 1013904223U;
    return manager->random_state;
}

static uint32_t random_between(pet_behavior_manager_t *manager, uint32_t minimum,
                               uint32_t maximum)
{
    uint32_t span = maximum - minimum;
    if (span == 0U) {
        return minimum;
    }
    return minimum + next_random(manager) % (span + 1U);
}

static void schedule_blink(pet_behavior_manager_t *manager, uint32_t now_ms)
{
    manager->next_blink_ms =
        add_clamped(now_ms,
                    random_between(manager,
                                   manager->config.auto_blink_min_interval_ms,
                                   manager->config.auto_blink_max_interval_ms));
}

static void schedule_look(pet_behavior_manager_t *manager,
                          const pet_behavior_context_t *context)
{
    uint32_t inactive_ms = context->now_ms - context->last_user_activity_ms;
    uint32_t remaining_inactive_ms =
        inactive_ms < manager->config.idle_look_inactivity_ms
            ? manager->config.idle_look_inactivity_ms - inactive_ms
            : 0U;
    uint32_t random_delay_ms =
        random_between(manager, manager->config.idle_look_min_interval_ms,
                       manager->config.idle_look_max_interval_ms);
    manager->next_look_ms =
        add_clamped(context->now_ms,
                    add_clamped(remaining_inactive_ms, random_delay_ms));
}

static void schedule_idle(pet_behavior_manager_t *manager,
                          const pet_behavior_context_t *context)
{
    manager->idle_scheduled = true;
    schedule_blink(manager, context->now_ms);
    schedule_look(manager, context);
}

static void make_autonomous_event(pet_event_t *event, pet_event_type_t type,
                                  uint32_t now_ms)
{
    *event = (pet_event_t){ .type = type,
                            .timestamp_ms = now_ms,
                            .flags = PET_EVENT_FLAG_AUTONOMOUS };
}

pet_status_t pet_behavior_manager_init(pet_behavior_manager_t *manager,
                                       const pet_behavior_config_t *config)
{
    if (manager == NULL || !behavior_config_valid(config)) {
        return PET_STATUS_INVALID_ARGUMENT;
    }
    manager->config = *config;
    manager->observed_state = PET_STATE_COUNT;
    manager->observed_user_activity_ms = 0U;
    manager->random_state = config->random_seed == 0U ? 1U : config->random_seed;
    manager->next_blink_ms = 0U;
    manager->next_look_ms = 0U;
    manager->initialized = true;
    manager->idle_scheduled = false;
    return PET_STATUS_OK;
}

bool pet_behavior_manager_update(pet_behavior_manager_t *manager,
                                 const pet_behavior_context_t *context,
                                 pet_event_t *event)
{
    bool state_changed;
    bool user_activity_changed;

    if (manager == NULL || context == NULL || event == NULL ||
        !manager->initialized || context->state >= PET_STATE_COUNT) {
        return false;
    }

    state_changed = context->state != manager->observed_state;
    user_activity_changed =
        context->last_user_activity_ms != manager->observed_user_activity_ms;
    if (state_changed) {
        manager->observed_state = context->state;
        manager->idle_scheduled = false;
    }
    if (user_activity_changed) {
        manager->observed_user_activity_ms = context->last_user_activity_ms;
    }
    if ((state_changed || user_activity_changed) && context->state == PET_STATE_IDLE) {
        schedule_idle(manager, context);
    }
    if (context->state != PET_STATE_IDLE || !manager->idle_scheduled) {
        return false;
    }

    if (context->now_ms - context->last_user_activity_ms >=
            manager->config.idle_look_inactivity_ms &&
        time_reached(context->now_ms, manager->next_look_ms)) {
        make_autonomous_event(event, PET_EVENT_LOOK, context->now_ms);
        event->data.look_direction =
            (next_random(manager) & 1U) == 0U ? PET_LOOK_LEFT : PET_LOOK_RIGHT;
        schedule_look(manager, context);
        return true;
    }
    if (time_reached(context->now_ms, manager->next_blink_ms)) {
        make_autonomous_event(event, PET_EVENT_BLINK, context->now_ms);
        schedule_blink(manager, context->now_ms);
        return true;
    }
    return false;
}

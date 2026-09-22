#include "pet/pet_core.h"

static const pet_animation_id_t state_animations[PET_STATE_COUNT] = {
    [PET_STATE_BOOT] = PET_ANIM_BOOT,
    [PET_STATE_IDLE] = PET_ANIM_IDLE,
    [PET_STATE_BLINK] = PET_ANIM_BLINK,
    [PET_STATE_LOOK_LEFT] = PET_ANIM_LOOK_LEFT,
    [PET_STATE_LOOK_RIGHT] = PET_ANIM_LOOK_RIGHT,
    [PET_STATE_HAPPY] = PET_ANIM_HAPPY,
    [PET_STATE_SLEEP] = PET_ANIM_SLEEP
};

static void enter_state(pet_core_t *core, pet_state_t state)
{
    core->state = state;
    core->state_elapsed_ms = 0U;
    core->requested_animation = state_animations[state];
    core->animation_request_pending = true;
}

pet_status_t pet_core_init(pet_core_t *core, const pet_core_config_t *config)
{
    if (core == NULL || config == NULL || config->boot_duration_ms == 0U) {
        return PET_STATUS_INVALID_ARGUMENT;
    }
    enter_state(core, PET_STATE_BOOT);
    return PET_STATUS_OK;
}

bool pet_core_handle_event(pet_core_t *core,
                           const pet_core_config_t *config,
                           const pet_event_t *event)
{
    if (core == NULL || config == NULL || event == NULL || core->state >= PET_STATE_COUNT ||
        event->type <= PET_EVENT_NONE || event->type >= PET_EVENT_COUNT) {
        return false;
    }

    switch (event->type) {
    case PET_EVENT_TIMER:
        core->state_elapsed_ms += event->data.timer_delta_ms;
        if (core->state == PET_STATE_BOOT &&
            core->state_elapsed_ms >= config->boot_duration_ms) {
            enter_state(core, PET_STATE_IDLE);
        }
        return true;
    case PET_EVENT_BUTTON:
        if (core->state == PET_STATE_SLEEP) {
            enter_state(core, PET_STATE_IDLE);
        } else {
            enter_state(core, PET_STATE_HAPPY);
        }
        return true;
    case PET_EVENT_MESSAGE:
    case PET_EVENT_HAPPY:
        if (core->state != PET_STATE_SLEEP) {
            enter_state(core, PET_STATE_HAPPY);
        }
        return true;
    case PET_EVENT_BLINK:
        if (core->state == PET_STATE_IDLE) {
            enter_state(core, PET_STATE_BLINK);
        }
        return true;
    case PET_EVENT_LOOK:
        if (event->data.look_direction != PET_LOOK_LEFT &&
            event->data.look_direction != PET_LOOK_RIGHT) {
            return false;
        }
        if (core->state != PET_STATE_SLEEP) {
            if (event->data.look_direction == PET_LOOK_LEFT) {
                enter_state(core, PET_STATE_LOOK_LEFT);
            } else {
                enter_state(core, PET_STATE_LOOK_RIGHT);
            }
        }
        return true;
    case PET_EVENT_NAV_NEXT:
        if (core->state == PET_STATE_SLEEP) {
            enter_state(core, PET_STATE_IDLE);
        } else if (core->state == PET_STATE_IDLE) {
            enter_state(core, PET_STATE_LOOK_RIGHT);
        }
        return true;
    case PET_EVENT_NAV_PREV:
        if (core->state == PET_STATE_SLEEP) {
            enter_state(core, PET_STATE_IDLE);
        } else if (core->state == PET_STATE_IDLE) {
            enter_state(core, PET_STATE_LOOK_LEFT);
        }
        return true;
    case PET_EVENT_SLEEP:
        enter_state(core, PET_STATE_SLEEP);
        return true;
    case PET_EVENT_WAKE:
        if (core->state == PET_STATE_SLEEP) {
            enter_state(core, PET_STATE_IDLE);
        }
        return true;
    case PET_EVENT_ANIMATION_DONE:
        if ((pet_animation_id_t)event->data.code == state_animations[core->state] &&
            (core->state == PET_STATE_BOOT || core->state == PET_STATE_BLINK ||
             core->state == PET_STATE_LOOK_LEFT || core->state == PET_STATE_LOOK_RIGHT ||
             core->state == PET_STATE_HAPPY)) {
            enter_state(core, PET_STATE_IDLE);
        }
        return true;
    case PET_EVENT_SENSOR:
    case PET_EVENT_NETWORK:
    case PET_EVENT_SYSTEM:
        return true;
    case PET_EVENT_NONE:
    case PET_EVENT_COUNT:
        break;
    }
    return false;
}

pet_state_t pet_core_state(const pet_core_t *core)
{
    return core == NULL || core->state >= PET_STATE_COUNT ? PET_STATE_COUNT : core->state;
}

bool pet_core_take_animation_request(pet_core_t *core, pet_animation_id_t *animation)
{
    if (core == NULL || animation == NULL || !core->animation_request_pending) {
        return false;
    }
    *animation = core->requested_animation;
    core->animation_request_pending = false;
    return true;
}

const char *pet_state_name(pet_state_t state)
{
    static const char *const names[PET_STATE_COUNT] = {
        "boot", "idle", "blink", "look_left", "look_right", "happy", "sleep"
    };
    return state < PET_STATE_COUNT ? names[state] : "invalid";
}

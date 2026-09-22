#include <assert.h>
#include <stdio.h>

#include "config/pet_config.h"
#include "pet/pet_core.h"

static pet_event_t timer_event(uint32_t delta_ms)
{
    pet_event_t event = { .type = PET_EVENT_TIMER,
                          .timestamp_ms = delta_ms,
                          .data.timer_delta_ms = delta_ms };
    return event;
}

int main(void)
{
    pet_core_t core;
    pet_core_config_t config = { .boot_duration_ms = 500U,
                                 .inactivity_sleep_ms =
                                     PET_CONFIG_DEFAULT_INACTIVITY_SLEEP_MS };
    pet_animation_id_t animation;
    pet_event_t event;

    assert(pet_core_init(&core, &config) == PET_STATUS_OK);
    assert(pet_core_state(&core) == PET_STATE_BOOT);
    assert(pet_core_take_animation_request(&core, &animation));
    assert(animation == PET_ANIM_BOOT);

    event = timer_event(499U);
    assert(pet_core_handle_event(&core, &config, &event));
    assert(pet_core_state(&core) == PET_STATE_BOOT);
    event = timer_event(1U);
    assert(pet_core_handle_event(&core, &config, &event));
    assert(pet_core_state(&core) == PET_STATE_IDLE);

    event = timer_event(1000U);
    assert(pet_core_handle_event(&core, &config, &event));
    assert(pet_core_state(&core) == PET_STATE_IDLE);

    event = (pet_event_t){ .type = PET_EVENT_BLINK };
    assert(pet_core_handle_event(&core, &config, &event));
    assert(pet_core_state(&core) == PET_STATE_BLINK);
    event = (pet_event_t){ .type = PET_EVENT_ANIMATION_DONE,
                           .data.code = PET_ANIM_BLINK };
    assert(pet_core_handle_event(&core, &config, &event));
    assert(pet_core_state(&core) == PET_STATE_IDLE);

    event = (pet_event_t){ .type = PET_EVENT_BUTTON };
    assert(pet_core_handle_event(&core, &config, &event));
    assert(pet_core_state(&core) == PET_STATE_HAPPY);
    event = (pet_event_t){ .type = PET_EVENT_SLEEP };
    assert(pet_core_handle_event(&core, &config, &event));
    assert(pet_core_state(&core) == PET_STATE_SLEEP);
    event = (pet_event_t){ .type = PET_EVENT_MESSAGE };
    assert(pet_core_handle_event(&core, &config, &event));
    assert(pet_core_state(&core) == PET_STATE_SLEEP);
    event = (pet_event_t){ .type = PET_EVENT_LOOK, .data.look_direction = 0 };
    assert(!pet_core_handle_event(&core, &config, &event));
    event = (pet_event_t){ .type = PET_EVENT_WAKE };
    assert(pet_core_handle_event(&core, &config, &event));
    assert(pet_core_state(&core) == PET_STATE_IDLE);

    event = (pet_event_t){ .type = PET_EVENT_SLEEP };
    assert(pet_core_handle_event(&core, &config, &event));
    assert(pet_core_state(&core) == PET_STATE_SLEEP);
    event = (pet_event_t){ .type = PET_EVENT_BUTTON };
    assert(pet_core_handle_event(&core, &config, &event));
    assert(pet_core_state(&core) == PET_STATE_IDLE);

    event = (pet_event_t){ .type = PET_EVENT_LOOK,
                           .data.look_direction = PET_LOOK_LEFT };
    assert(pet_core_handle_event(&core, &config, &event));
    assert(pet_core_state(&core) == PET_STATE_LOOK_LEFT);
    event.data.look_direction = PET_LOOK_RIGHT;
    assert(pet_core_handle_event(&core, &config, &event));
    assert(pet_core_state(&core) == PET_STATE_LOOK_RIGHT);
    event.data.look_direction = 0;
    assert(!pet_core_handle_event(&core, &config, &event));
    event = (pet_event_t){ .type = PET_EVENT_HAPPY };
    assert(pet_core_handle_event(&core, &config, &event));
    assert(pet_core_state(&core) == PET_STATE_HAPPY);

    event = (pet_event_t){ .type = PET_EVENT_SLEEP };
    assert(pet_core_handle_event(&core, &config, &event));
    assert(pet_core_state(&core) == PET_STATE_SLEEP);
    event = (pet_event_t){ .type = PET_EVENT_NAV_NEXT };
    assert(pet_core_handle_event(&core, &config, &event));
    assert(pet_core_state(&core) == PET_STATE_IDLE);
    event = (pet_event_t){ .type = PET_EVENT_NAV_NEXT };
    assert(pet_core_handle_event(&core, &config, &event));
    assert(pet_core_state(&core) == PET_STATE_LOOK_RIGHT);
    event = (pet_event_t){ .type = PET_EVENT_ANIMATION_DONE,
                           .data.code = PET_ANIM_LOOK_RIGHT };
    assert(pet_core_handle_event(&core, &config, &event));
    assert(pet_core_state(&core) == PET_STATE_IDLE);
    event = (pet_event_t){ .type = PET_EVENT_NAV_PREV };
    assert(pet_core_handle_event(&core, &config, &event));
    assert(pet_core_state(&core) == PET_STATE_LOOK_LEFT);
    event = (pet_event_t){ .type = PET_EVENT_ANIMATION_DONE,
                           .data.code = PET_ANIM_LOOK_LEFT };
    assert(pet_core_handle_event(&core, &config, &event));
    assert(pet_core_state(&core) == PET_STATE_IDLE);
    event = (pet_event_t){ .type = PET_EVENT_SLEEP };
    assert(pet_core_handle_event(&core, &config, &event));
    assert(pet_core_state(&core) == PET_STATE_SLEEP);
    event = (pet_event_t){ .type = PET_EVENT_NAV_PREV };
    assert(pet_core_handle_event(&core, &config, &event));
    assert(pet_core_state(&core) == PET_STATE_IDLE);

    event = (pet_event_t){ .type = PET_EVENT_COUNT };
    assert(!pet_core_handle_event(&core, &config, &event));
    core.state = PET_STATE_COUNT;
    event = timer_event(1U);
    assert(!pet_core_handle_event(&core, &config, &event));

    (void)printf("test_pet_core: ok\n");
    return 0;
}

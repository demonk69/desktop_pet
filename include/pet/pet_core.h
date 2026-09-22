#ifndef PET_CORE_H
#define PET_CORE_H

#include <stdbool.h>
#include <stdint.h>

#include "animation/pet_animation.h"
#include "core/pet_event.h"
#include "pet/status.h"

typedef enum {
    PET_STATE_BOOT = 0,
    PET_STATE_IDLE,
    PET_STATE_BLINK,
    PET_STATE_LOOK_LEFT,
    PET_STATE_LOOK_RIGHT,
    PET_STATE_HAPPY,
    PET_STATE_SLEEP,
    PET_STATE_COUNT
} pet_state_t;

typedef struct {
    uint32_t boot_duration_ms;
    uint32_t inactivity_sleep_ms;
} pet_core_config_t;

typedef struct {
    pet_state_t state;
    uint32_t state_elapsed_ms;
    pet_animation_id_t requested_animation;
    bool animation_request_pending;
} pet_core_t;

pet_status_t pet_core_init(pet_core_t *core, const pet_core_config_t *config);
bool pet_core_handle_event(pet_core_t *core,
                           const pet_core_config_t *config,
                           const pet_event_t *event);
pet_state_t pet_core_state(const pet_core_t *core);
bool pet_core_take_animation_request(pet_core_t *core, pet_animation_id_t *animation);
const char *pet_state_name(pet_state_t state);

#endif

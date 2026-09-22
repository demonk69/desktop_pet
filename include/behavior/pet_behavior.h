#ifndef PET_BEHAVIOR_H
#define PET_BEHAVIOR_H

#include <stdbool.h>
#include <stdint.h>

#include "core/pet_event.h"
#include "pet/pet_core.h"
#include "pet/status.h"

typedef struct {
    uint32_t auto_blink_min_interval_ms;
    uint32_t auto_blink_max_interval_ms;
    uint32_t idle_look_inactivity_ms;
    uint32_t idle_look_min_interval_ms;
    uint32_t idle_look_max_interval_ms;
    uint32_t random_seed;
} pet_behavior_config_t;

typedef struct {
    uint32_t now_ms;
    uint32_t last_user_activity_ms;
    pet_state_t state;
} pet_behavior_context_t;

typedef struct {
    pet_behavior_config_t config;
    pet_state_t observed_state;
    uint32_t observed_user_activity_ms;
    uint32_t random_state;
    uint32_t next_blink_ms;
    uint32_t next_look_ms;
    bool initialized;
    bool idle_scheduled;
} pet_behavior_manager_t;

pet_status_t pet_behavior_manager_init(pet_behavior_manager_t *manager,
                                       const pet_behavior_config_t *config);
bool pet_behavior_manager_update(pet_behavior_manager_t *manager,
                                 const pet_behavior_context_t *context,
                                 pet_event_t *event);

#endif

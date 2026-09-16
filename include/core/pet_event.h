#ifndef PET_EVENT_H
#define PET_EVENT_H

#include <stdint.h>

typedef enum {
    PET_EVENT_NONE = 0,
    PET_EVENT_TIMER,
    PET_EVENT_BUTTON,
    PET_EVENT_SENSOR,
    PET_EVENT_MESSAGE,
    PET_EVENT_NETWORK,
    PET_EVENT_SLEEP,
    PET_EVENT_WAKE,
    PET_EVENT_ANIMATION_DONE,
    PET_EVENT_SYSTEM,
    PET_EVENT_COUNT
} pet_event_type_t;

typedef struct {
    pet_event_type_t type;
    uint32_t timestamp_ms;
    union {
        uint32_t timer_delta_ms;
        int32_t button_id;
        int32_t sensor_value;
        int32_t code;
    } data;
} pet_event_t;

#endif

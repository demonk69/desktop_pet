#ifndef PET_BACKLIGHT_H
#define PET_BACKLIGHT_H

#include <stdint.h>

#include "pet/status.h"

typedef struct {
    pet_status_t (*init)(void *context);
    pet_status_t (*set_brightness)(void *context, uint8_t percent);
    pet_status_t (*fade)(void *context, uint8_t percent, uint32_t duration_ms);
    pet_status_t (*sleep)(void *context);
    pet_status_t (*wake)(void *context);
} pet_backlight_ops_t;

typedef struct {
    void *context;
    const pet_backlight_ops_t *ops;
} pet_backlight_t;

pet_status_t pet_backlight_init(pet_backlight_t *backlight);
pet_status_t pet_backlight_set_brightness(pet_backlight_t *backlight, uint8_t percent);
pet_status_t pet_backlight_fade(pet_backlight_t *backlight, uint8_t percent,
                                uint32_t duration_ms);
pet_status_t pet_backlight_sleep(pet_backlight_t *backlight);
pet_status_t pet_backlight_wake(pet_backlight_t *backlight);

#endif

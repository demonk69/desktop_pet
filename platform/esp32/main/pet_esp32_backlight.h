#ifndef PET_ESP32_BACKLIGHT_H
#define PET_ESP32_BACKLIGHT_H

#include <stdbool.h>
#include <stdint.h>

#include "hal/backlight.h"

typedef struct {
    int gpio;
    bool active_high;
    uint32_t pwm_frequency_hz;
    uint8_t pwm_resolution_bits;
    uint8_t default_percent;
    uint8_t sleep_percent;
    uint8_t brightness;
    uint8_t brightness_before_sleep;
} pet_esp32_backlight_t;

pet_status_t pet_esp32_backlight_create(pet_esp32_backlight_t *backend,
                                         pet_backlight_t *backlight, int gpio,
                                         bool active_high,
                                         uint32_t pwm_frequency_hz,
                                         uint8_t pwm_resolution_bits,
                                         uint8_t default_percent,
                                         uint8_t sleep_percent);

#endif

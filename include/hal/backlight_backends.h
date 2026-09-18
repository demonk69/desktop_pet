#ifndef PET_BACKLIGHT_BACKENDS_H
#define PET_BACKLIGHT_BACKENDS_H

#include "config/pet_config.h"
#include "hal/backlight.h"

/* Generic PWM stub; V0.3 ESP32 GPIO on/off backend lives under platform/esp32. */
pet_status_t pet_backlight_create_pwm_stub(pet_backlight_t *backlight,
                                           const pet_hardware_config_t *config);

#endif

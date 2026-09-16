#ifndef PET_BACKLIGHT_BACKENDS_H
#define PET_BACKLIGHT_BACKENDS_H

#include "config/pet_config.h"
#include "hal/backlight.h"

/* HW_VERIFY: implemented after the MCU PWM peripheral and GPIO are confirmed. */
pet_status_t pet_backlight_create_pwm_stub(pet_backlight_t *backlight,
                                           const pet_hardware_config_t *config);

#endif

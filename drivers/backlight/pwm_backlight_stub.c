#include "hal/backlight_backends.h"

pet_status_t pet_backlight_create_pwm_stub(pet_backlight_t *backlight,
                                           const pet_hardware_config_t *config)
{
    (void)backlight;
    (void)config;
    /* HW_VERIFY: PWM peripheral, polarity, frequency, and GPIO are unknown. */
    return PET_STATUS_NOT_SUPPORTED;
}

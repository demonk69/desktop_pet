#include "hal/backlight_backends.h"

pet_status_t pet_backlight_create_pwm_stub(pet_backlight_t *backlight,
                                           const pet_hardware_config_t *config)
{
    (void)backlight;
    (void)config;
    /* HW_VERIFY: LEDC frequency and fade policy; V0.3 uses ESP32 GPIO on/off. */
    return PET_STATUS_NOT_SUPPORTED;
}

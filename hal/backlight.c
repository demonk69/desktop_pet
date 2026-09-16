#include "hal/backlight.h"

#include <stdbool.h>
#include <stddef.h>

static bool valid(const pet_backlight_t *backlight)
{
    return backlight != NULL && backlight->ops != NULL;
}

pet_status_t pet_backlight_init(pet_backlight_t *backlight)
{
    return valid(backlight) && backlight->ops->init != NULL
               ? backlight->ops->init(backlight->context)
               : PET_STATUS_NOT_SUPPORTED;
}

pet_status_t pet_backlight_set_brightness(pet_backlight_t *backlight, uint8_t percent)
{
    if (!valid(backlight) || backlight->ops->set_brightness == NULL || percent > 100U) {
        return PET_STATUS_INVALID_ARGUMENT;
    }
    return backlight->ops->set_brightness(backlight->context, percent);
}

pet_status_t pet_backlight_fade(pet_backlight_t *backlight, uint8_t percent,
                                uint32_t duration_ms)
{
    if (!valid(backlight) || backlight->ops->fade == NULL || percent > 100U) {
        return PET_STATUS_INVALID_ARGUMENT;
    }
    return backlight->ops->fade(backlight->context, percent, duration_ms);
}

pet_status_t pet_backlight_sleep(pet_backlight_t *backlight)
{
    return valid(backlight) && backlight->ops->sleep != NULL
               ? backlight->ops->sleep(backlight->context)
               : PET_STATUS_NOT_SUPPORTED;
}

pet_status_t pet_backlight_wake(pet_backlight_t *backlight)
{
    return valid(backlight) && backlight->ops->wake != NULL
               ? backlight->ops->wake(backlight->context)
               : PET_STATUS_NOT_SUPPORTED;
}

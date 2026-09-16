#include "hal/display_backends.h"

pet_status_t pet_display_create_i8080_stub(pet_display_t *display,
                                           const pet_hardware_config_t *config)
{
    (void)display;
    (void)config;
    /* HW_VERIFY: data width, write timing, and GPIO mapping are unknown. */
    return PET_STATUS_NOT_SUPPORTED;
}

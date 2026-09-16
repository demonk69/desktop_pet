#include "hal/display_backends.h"

pet_status_t pet_display_create_st7789_stub(pet_display_t *display,
                                            const pet_hardware_config_t *config)
{
    (void)display;
    (void)config;
    /* HW_VERIFY: initialization sequence, offsets, color order, and reset timing. */
    return PET_STATUS_NOT_SUPPORTED;
}

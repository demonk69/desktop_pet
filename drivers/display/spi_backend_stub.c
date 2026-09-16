#include "hal/display_backends.h"

pet_status_t pet_display_create_spi_stub(pet_display_t *display,
                                         const pet_hardware_config_t *config)
{
    (void)display;
    (void)config;
    /* HW_VERIFY: bind to the selected MCU SDK SPI API after board identification. */
    return PET_STATUS_NOT_SUPPORTED;
}

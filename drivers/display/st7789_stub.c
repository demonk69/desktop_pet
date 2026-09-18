#include "hal/display_backends.h"

pet_status_t pet_display_create_st7789_stub(pet_display_t *display,
                                            const pet_hardware_config_t *config)
{
    (void)display;
    (void)config;
    /* The verified ESP-IDF ST7789 implementation lives under platform/esp32. */
    return PET_STATUS_NOT_SUPPORTED;
}

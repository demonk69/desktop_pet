#include <assert.h>
#include <stdio.h>

#include "config/pet_config.h"

int main(void)
{
    pet_config_t config;

    pet_config_set_development_defaults(&config);
    assert(pet_config_validate(&config) == PET_STATUS_OK);
    assert(config.hardware.gpio_mosi == PET_GPIO_HW_VERIFY);
    assert(config.hardware.spi_frequency_hz == PET_VALUE_HW_VERIFY);
    assert(!config.hardware.psram_enabled);

    config.hardware.framebuffer_bytes = 1U;
    assert(pet_config_validate(&config) == PET_STATUS_INVALID_ARGUMENT);
    pet_config_set_development_defaults(&config);
    config.hardware.rotation = 4U;
    assert(pet_config_validate(&config) == PET_STATUS_INVALID_ARGUMENT);
    assert(pet_config_validate(NULL) == PET_STATUS_INVALID_ARGUMENT);

    (void)printf("test_config: ok\n");
    return 0;
}

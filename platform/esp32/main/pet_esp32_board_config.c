#include "pet_esp32_board_config.h"

#include <string.h>

#if !defined(PET_LCD_SPI_FREQUENCY_HZ) || !defined(PET_LCD_STAGING_BUFFER_SIZE) || \
    !defined(PET_LCD_DMA_ENABLED) || !defined(PET_LCD_BLOCK_HEIGHT)
#error "ESP32 LCD build configuration is incomplete"
#endif

void pet_esp32_board_config_init(pet_esp32_board_config_t *config)
{
    if (config == NULL) {
        return;
    }
    memset(config, 0, sizeof(*config));
    pet_config_set_development_defaults(&config->pet);

    /* Values below are copied from the hardware-verified LCD bring-up project. */
    config->pet.hardware.width = 240U;
    config->pet.hardware.height = 240U;
    config->pet.hardware.x_offset = 0U;
    config->pet.hardware.y_offset = 0U;
    config->pet.hardware.rotation = 0U;
    config->pet.hardware.bus = PET_LCD_BUS_SPI;
    config->pet.hardware.color_order = PET_LCD_COLOR_RGB;
    config->pet.hardware.spi_frequency_hz = PET_LCD_SPI_FREQUENCY_HZ;
    config->pet.hardware.gpio_backlight = 7;
    config->pet.hardware.gpio_reset = 8;
    config->pet.hardware.gpio_dc = 9;
    config->pet.hardware.gpio_cs = 10;
    config->pet.hardware.gpio_mosi = 11;
    config->pet.hardware.gpio_sclk = 12;
    config->pet.hardware.backlight_pwm_hz = PET_VALUE_HW_VERIFY;
    config->pet.hardware.psram_enabled = true;
    config->pet.hardware.framebuffer_bytes = 240U * 240U * sizeof(uint16_t);

    config->lcd_spi_host = SPI2_HOST;
    config->lcd_spi_mode = 0U;
    config->lcd_madctl = 0x00U;
    config->lcd_inverted = false;
    config->backlight_active_high = true;
    config->lcd_dma_enabled = PET_LCD_DMA_ENABLED != 0;
    config->staging_buffer_bytes = PET_LCD_STAGING_BUFFER_SIZE;
    config->block_height = PET_LCD_BLOCK_HEIGHT;
    config->reset_low_ms = 20U;
    config->reset_high_ms = 120U;
}

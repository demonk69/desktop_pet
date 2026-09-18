#include "pet_esp32_board_config.h"

#include <string.h>

#if !defined(PET_LCD_SPI_FREQUENCY_HZ) || !defined(PET_LCD_STAGING_BUFFER_SIZE) || \
    !defined(PET_LCD_DMA_ENABLED) || !defined(PET_LCD_BLOCK_HEIGHT) ||            \
    !defined(PET_LCD_DIAG_MODE) || !defined(PET_LCD_DIAG_TRANSPORT)
#error "ESP32 LCD build configuration is incomplete"
#endif

#define PET_STRINGIFY_INNER(value) #value
#define PET_STRINGIFY(value)        PET_STRINGIFY_INNER(value)

/* Forced transport parameters for A/B visual diagnostics. */
#define PET_DIAG_V04_SPI_HZ       (10U * 1000U * 1000U)
#define PET_DIAG_V04_STAGING      64U
#define PET_DIAG_V05_SPI_HZ       (40U * 1000U * 1000U)
#define PET_DIAG_V05_STAGING      4096U
#define PET_DIAG_ROW8_SPI_HZ      (40U * 1000U * 1000U)
#define PET_DIAG_ROW8_STAGING     3840U
#define PET_DIAG_ROW8_BLOCK_LINES 8U

static bool diag_mode_is(const char *name)
{
    return strcmp(PET_STRINGIFY(PET_LCD_DIAG_MODE), name) == 0;
}

static bool diag_transport_is(const char *name)
{
    return strcmp(PET_STRINGIFY(PET_LCD_DIAG_TRANSPORT), name) == 0;
}

const char *pet_esp32_diag_mode_name(void)
{
    return PET_STRINGIFY(PET_LCD_DIAG_MODE);
}

const char *pet_esp32_diag_transport_name(void)
{
    return PET_STRINGIFY(PET_LCD_DIAG_TRANSPORT);
}

bool pet_esp32_diag_mode_is_valid(void)
{
    return diag_mode_is("NONE") || diag_mode_is("STATIC_ONCE") ||
           diag_mode_is("STATIC_REPEAT");
}

bool pet_esp32_diag_transport_is_valid(void)
{
    return diag_transport_is("DEFAULT") || diag_transport_is("V04") ||
           diag_transport_is("V05") || diag_transport_is("ROW8");
}

bool pet_esp32_diag_is_static(void)
{
    return diag_mode_is("STATIC_ONCE") || diag_mode_is("STATIC_REPEAT");
}

bool pet_esp32_diag_static_repeat(void)
{
    return diag_mode_is("STATIC_REPEAT");
}

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
    config->reset_low_ms = 20U;
    config->reset_high_ms = 120U;

    /* Diagnostic transports force only LCD link parameters. The selected mode
       still independently decides whether to run a static frame or animation. */
    if (diag_transport_is("V04")) {
        config->pet.hardware.spi_frequency_hz = PET_DIAG_V04_SPI_HZ;
        config->lcd_dma_enabled = false;
        config->staging_buffer_bytes = PET_DIAG_V04_STAGING;
        config->block_height = 0U;
    } else if (diag_transport_is("V05")) {
        config->pet.hardware.spi_frequency_hz = PET_DIAG_V05_SPI_HZ;
        config->lcd_dma_enabled = true;
        config->staging_buffer_bytes = PET_DIAG_V05_STAGING;
        config->block_height = 0U;
    } else if (diag_transport_is("ROW8")) {
        config->pet.hardware.spi_frequency_hz = PET_DIAG_ROW8_SPI_HZ;
        config->lcd_dma_enabled = true;
        config->staging_buffer_bytes = PET_DIAG_ROW8_STAGING;
        config->block_height = PET_DIAG_ROW8_BLOCK_LINES;
    } else {
        config->pet.hardware.spi_frequency_hz = PET_LCD_SPI_FREQUENCY_HZ;
        config->lcd_dma_enabled = PET_LCD_DMA_ENABLED != 0;
        config->staging_buffer_bytes = PET_LCD_STAGING_BUFFER_SIZE;
        config->block_height = PET_LCD_BLOCK_HEIGHT;
    }
}

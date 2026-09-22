#include "config/pet_config.h"

void pet_config_set_development_defaults(pet_config_t *config)
{
    if (config == NULL) {
        return;
    }
    config->app.core.boot_duration_ms = 600U;
    config->app.core.inactivity_sleep_ms = PET_CONFIG_DEFAULT_INACTIVITY_SLEEP_MS;
    config->app.behavior.auto_blink_min_interval_ms = 3000U;
    config->app.behavior.auto_blink_max_interval_ms = 8000U;
    config->app.behavior.idle_look_inactivity_ms = 9000U;
    config->app.behavior.idle_look_min_interval_ms = 1000U;
    config->app.behavior.idle_look_max_interval_ms = 3000U;
    config->app.behavior.random_seed = 1U;
    config->app.animations = pet_builtin_animation_catalog();

    /* Host defaults; the verified ESP32 board config overrides hardware fields. */
    config->hardware.width = 240U;
    config->hardware.height = 240U;
    config->hardware.x_offset = PET_VALUE_HW_VERIFY;
    config->hardware.y_offset = PET_VALUE_HW_VERIFY;
    config->hardware.rotation = 0U;
    config->hardware.bus = PET_LCD_BUS_SPI;
    config->hardware.color_order = PET_LCD_COLOR_RGB;
    config->hardware.spi_frequency_hz = PET_VALUE_HW_VERIFY;
    config->hardware.gpio_mosi = PET_GPIO_HW_VERIFY;
    config->hardware.gpio_sclk = PET_GPIO_HW_VERIFY;
    config->hardware.gpio_cs = PET_GPIO_HW_VERIFY;
    config->hardware.gpio_dc = PET_GPIO_HW_VERIFY;
    config->hardware.gpio_reset = PET_GPIO_HW_VERIFY;
    config->hardware.gpio_backlight = PET_GPIO_HW_VERIFY;
    config->hardware.backlight_pwm_hz = PET_VALUE_HW_VERIFY;
    config->hardware.psram_enabled = false;
    config->hardware.framebuffer_bytes =
        (size_t)config->hardware.width * config->hardware.height * sizeof(uint16_t);
}

pet_status_t pet_config_validate(const pet_config_t *config)
{
    size_t minimum_framebuffer;
    if (config == NULL || config->app.animations == NULL ||
        config->app.core.boot_duration_ms == 0U ||
        config->app.core.inactivity_sleep_ms == 0U ||
        config->app.behavior.auto_blink_min_interval_ms == 0U ||
        config->app.behavior.auto_blink_min_interval_ms >
            config->app.behavior.auto_blink_max_interval_ms ||
        config->app.behavior.idle_look_inactivity_ms == 0U ||
        config->app.behavior.idle_look_min_interval_ms == 0U ||
        config->app.behavior.idle_look_min_interval_ms >
            config->app.behavior.idle_look_max_interval_ms ||
        config->hardware.width == 0U || config->hardware.height == 0U ||
        config->hardware.rotation > 3U) {
        return PET_STATUS_INVALID_ARGUMENT;
    }
    minimum_framebuffer =
        (size_t)config->hardware.width * config->hardware.height * sizeof(uint16_t);
    if (config->hardware.framebuffer_bytes < minimum_framebuffer) {
        return PET_STATUS_INVALID_ARGUMENT;
    }
    return PET_STATUS_OK;
}

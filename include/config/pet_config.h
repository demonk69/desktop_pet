#ifndef PET_CONFIG_H
#define PET_CONFIG_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "pet/pet_app.h"
#include "pet/status.h"

#define PET_GPIO_HW_VERIFY (-1)
#define PET_VALUE_HW_VERIFY (0U)
#define PET_CONFIG_DEFAULT_INACTIVITY_SLEEP_MS 15000U

typedef enum {
    PET_LCD_BUS_SPI = 0,
    PET_LCD_BUS_I8080
} pet_lcd_bus_t;

typedef enum {
    PET_LCD_COLOR_RGB = 0,
    PET_LCD_COLOR_BGR
} pet_lcd_color_order_t;

typedef struct {
    uint16_t width;
    uint16_t height;
    uint16_t x_offset;
    uint16_t y_offset;
    uint8_t rotation;
    pet_lcd_bus_t bus;
    pet_lcd_color_order_t color_order;
    uint32_t spi_frequency_hz;
    int gpio_mosi;
    int gpio_sclk;
    int gpio_cs;
    int gpio_dc;
    int gpio_reset;
    int gpio_backlight;
    uint32_t backlight_pwm_hz;
    bool psram_enabled;
    size_t framebuffer_bytes;
} pet_hardware_config_t;

typedef struct {
    pet_app_config_t app;
    pet_hardware_config_t hardware;
} pet_config_t;

void pet_config_set_development_defaults(pet_config_t *config);
pet_status_t pet_config_validate(const pet_config_t *config);

#endif

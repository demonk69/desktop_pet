#ifndef PET_ESP32_BOARD_CONFIG_H
#define PET_ESP32_BOARD_CONFIG_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "driver/spi_master.h"

#include "config/pet_config.h"

typedef struct {
    pet_config_t pet;
    spi_host_device_t lcd_spi_host;
    uint8_t lcd_spi_mode;
    uint8_t lcd_madctl;
    bool lcd_inverted;
    bool backlight_active_high;
    bool lcd_dma_enabled;
    size_t staging_buffer_bytes;
    size_t block_height;
    int rotary_ga_gpio;
    int rotary_bb_gpio;
    const char *rotary_ga_pull;
    const char *rotary_bb_pull;
    uint8_t rotary_transitions_per_detent;
    uint32_t rotary_adc_low_max_mv;
    uint32_t rotary_adc_press_min_mv;
    uint32_t rotary_adc_press_max_mv;
    uint32_t rotary_adc_high_min_mv;
    uint32_t rotary_press_debounce_ms;
    uint8_t backlight_pwm_resolution_bits;
    uint8_t backlight_default_percent;
    uint8_t backlight_sleep_percent;
    uint32_t reset_low_ms;
    uint32_t reset_high_ms;
} pet_esp32_board_config_t;

void pet_esp32_board_config_init(pet_esp32_board_config_t *config);

const char *pet_esp32_diag_mode_name(void);
const char *pet_esp32_diag_transport_name(void);
bool pet_esp32_diag_mode_is_valid(void);
bool pet_esp32_diag_transport_is_valid(void);
bool pet_esp32_diag_is_static(void);
bool pet_esp32_diag_static_repeat(void);

#endif

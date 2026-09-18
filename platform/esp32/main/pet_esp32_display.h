#ifndef PET_ESP32_DISPLAY_H
#define PET_ESP32_DISPLAY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "driver/spi_master.h"

#include "hal/display.h"
#include "pet_esp32_board_config.h"

typedef struct {
    pet_esp32_board_config_t config;
    spi_device_handle_t spi;
    uint16_t *framebuffer;
    uint8_t *transfer_buffer;
    uint64_t last_flush_us;
    uint32_t actual_spi_frequency_hz;
    bool bus_initialized;
    bool initialized;
} pet_esp32_display_t;

pet_status_t pet_esp32_display_create(pet_esp32_display_t *backend,
                                      pet_display_t *display,
                                      const pet_esp32_board_config_t *config);
uint64_t pet_esp32_display_last_flush_us(const pet_esp32_display_t *backend);
uint32_t pet_esp32_display_actual_frequency_hz(const pet_esp32_display_t *backend);

#endif

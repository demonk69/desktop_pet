#ifndef PET_ESP32_ROTARY_H
#define PET_ESP32_ROTARY_H

#include <stdbool.h>
#include <stdint.h>

#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_oneshot.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "pet/pet_app.h"
#include "pet/status.h"
#include "pet_esp32_board_config.h"

typedef enum {
    PET_ESP32_ROTARY_NONE = 0,
    PET_ESP32_ROTARY_CW,
    PET_ESP32_ROTARY_CCW,
} pet_esp32_rotary_direction_t;

typedef enum {
    PET_ESP32_ROTARY_GA_LOW = 0,
    PET_ESP32_ROTARY_GA_MID,
    PET_ESP32_ROTARY_GA_HIGH,
    PET_ESP32_ROTARY_GA_UNCERTAIN,
} pet_esp32_rotary_ga_class_t;

typedef enum {
    PET_ESP32_ROTARY_PRESS_RELEASED = 0,
    PET_ESP32_ROTARY_PRESS_CANDIDATE,
    PET_ESP32_ROTARY_PRESS_PRESSED,
    PET_ESP32_ROTARY_PRESS_RELEASE_CANDIDATE,
} pet_esp32_rotary_press_state_t;

typedef struct {
    uint8_t previous_ab;
    uint8_t transitions_per_detent;
    int8_t accumulator;
    uint32_t cw_count;
    uint32_t ccw_count;
    uint32_t invalid_transition_count;
    bool initialized;
} pet_esp32_rotary_decoder_t;

typedef struct {
    uint64_t timestamp_us;
    uint8_t ab;
} pet_esp32_rotary_edge_t;

typedef struct {
    int ga_gpio;
    int bb_gpio;
    const char *ga_pull;
    const char *bb_pull;
    uint8_t transitions_per_detent;
    uint32_t adc_low_max_mv;
    uint32_t adc_press_min_mv;
    uint32_t adc_press_max_mv;
    uint32_t adc_high_min_mv;
    uint32_t press_debounce_ms;
    pet_esp32_rotary_decoder_t decoder;
    QueueHandle_t raw_edges;
    QueueHandle_t events;
    TaskHandle_t task;
    adc_oneshot_unit_handle_t adc_unit;
    adc_cali_handle_t adc_cali;
    bool adc_calibration_enabled;
    adc_unit_t adc_unit_id;
    adc_channel_t adc_channel;
    pet_esp32_rotary_press_state_t press_state;
    uint64_t press_phase_start_us;
    uint64_t last_adc_us;
    uint32_t press_count;
    uint32_t dropped_event_count;
    uint64_t last_stats_us;
    bool enabled;
} pet_esp32_rotary_t;

void pet_esp32_rotary_decoder_init(pet_esp32_rotary_decoder_t *decoder,
                                   uint8_t initial_ab,
                                   uint8_t transitions_per_detent);
pet_esp32_rotary_direction_t pet_esp32_rotary_decoder_update(
    pet_esp32_rotary_decoder_t *decoder, uint8_t current_ab);

pet_status_t pet_esp32_rotary_create(pet_esp32_rotary_t *backend,
                                     const pet_esp32_board_config_t *config);
pet_status_t pet_esp32_rotary_start(pet_esp32_rotary_t *backend);
void pet_esp32_rotary_drain_events(pet_esp32_rotary_t *backend, pet_app_t *app);
bool pet_esp32_rotary_is_enabled(const pet_esp32_rotary_t *backend);

#endif

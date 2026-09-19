#include "pet_esp32_rotary.h"

#include <stddef.h>
#include <string.h>

#include "driver/gpio.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_attr.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#define ROTARY_RAW_QUEUE_LENGTH   96U
#define ROTARY_EVENT_QUEUE_LENGTH 32U
#define ROTARY_TASK_STACK_WORDS   3072U
#define ROTARY_TASK_PRIORITY      10
#define ROTARY_STATS_PERIOD_US    5000000ULL
#define ROTARY_EDGE_ADC_MIN_US    2000ULL
#define ROTARY_PRESS_ADC_MIN_US   5000ULL
#define ROTARY_ADC_ATTEN          ADC_ATTEN_DB_12

static const char *TAG = "rotary";

/* Verified CW sequence 11->10->00->01->11 maps to +1 steps; the CCW sequence
 * 11->01->00->10->11 maps to -1 steps. Bounce (5~146 us) alternates between
 * +1/-1 and cancels in the signed accumulator. */
static const int8_t transition_table[16] DRAM_ATTR = {
    0,  1, -1, 0,
   -1,  0,  0, 1,
    1,  0,  0, -1,
    0, -1,  1, 0,
};

static int8_t quadrature_delta(uint8_t previous_ab, uint8_t current_ab)
{
    return transition_table[((previous_ab & 0x3U) << 2U) | (current_ab & 0x3U)];
}

static bool pull_is_verified(const char *pull)
{
    return pull != NULL && strcmp(pull, "HW_VERIFY") != 0;
}

static bool config_is_verified(const pet_esp32_rotary_t *backend)
{
    return backend != NULL && GPIO_IS_VALID_GPIO(backend->ga_gpio) &&
           GPIO_IS_VALID_GPIO(backend->bb_gpio) && pull_is_verified(backend->ga_pull) &&
           pull_is_verified(backend->bb_pull) && backend->transitions_per_detent > 0U;
}

static pet_status_t pull_modes(const char *pull, bool *pull_up, bool *pull_down)
{
    if (pull == NULL || pull_up == NULL || pull_down == NULL) {
        return PET_STATUS_INVALID_ARGUMENT;
    }
    if (strcmp(pull, "NONE") == 0) {
        *pull_up = false;
        *pull_down = false;
        return PET_STATUS_OK;
    }
    if (strcmp(pull, "UP") == 0) {
        *pull_up = true;
        *pull_down = false;
        return PET_STATUS_OK;
    }
    if (strcmp(pull, "DOWN") == 0) {
        *pull_up = false;
        *pull_down = true;
        return PET_STATUS_OK;
    }
    return PET_STATUS_INVALID_ARGUMENT;
}

static pet_status_t configure_pin(int gpio, bool pull_up, bool pull_down)
{
    gpio_config_t config;
    if (!GPIO_IS_VALID_GPIO(gpio)) {
        return PET_STATUS_INVALID_ARGUMENT;
    }
    config = (gpio_config_t){ .pin_bit_mask = 1ULL << gpio,
                              .mode = GPIO_MODE_INPUT,
                              .pull_up_en = pull_up ? GPIO_PULLUP_ENABLE
                                                    : GPIO_PULLUP_DISABLE,
                              .pull_down_en = pull_down ? GPIO_PULLDOWN_ENABLE
                                                        : GPIO_PULLDOWN_DISABLE,
                              .intr_type = GPIO_INTR_ANYEDGE };
    return gpio_config(&config) == ESP_OK ? PET_STATUS_OK : PET_STATUS_IO_ERROR;
}

static inline uint8_t read_ab(const pet_esp32_rotary_t *backend)
{
    return (uint8_t)(((gpio_get_level((gpio_num_t)backend->ga_gpio) != 0) ? 0x2U : 0U) |
                     ((gpio_get_level((gpio_num_t)backend->bb_gpio) != 0) ? 0x1U : 0U));
}

/* Minimal ISR: capture a raw AB sample with a timestamp and push it to the raw
 * edge queue. No quadrature decoding, ADC, Pet Core, Renderer, malloc, printf,
 * or Backlight HAL calls happen here. */
static void IRAM_ATTR rotary_edge_isr(void *arg)
{
    pet_esp32_rotary_t *backend = arg;
    pet_esp32_rotary_edge_t edge;
    BaseType_t wake = pdFALSE;
    if (backend == NULL || !backend->enabled || backend->raw_edges == NULL) {
        return;
    }
    edge.timestamp_us = (uint64_t)esp_timer_get_time();
    edge.ab = read_ab(backend);
    (void)xQueueSendFromISR(backend->raw_edges, &edge, &wake);
    if (wake == pdTRUE) {
        portYIELD_FROM_ISR();
    }
}

static void drain_raw_queue(pet_esp32_rotary_t *backend)
{
    pet_esp32_rotary_edge_t edge;
    while (backend->raw_edges != NULL &&
           xQueueReceive(backend->raw_edges, &edge, 0U) == pdTRUE) {
    }
}

/* Pauses digital capture, classifies the GA voltage through the ADC, restores
 * the digital input path, and re-synchronizes the decoder baseline from the
 * current pad levels. When drain is set, queued raw edges are discarded, which
 * is used to isolate press sessions from the quadrature accumulator. */
static pet_esp32_rotary_ga_class_t sample_ga_class(pet_esp32_rotary_t *backend,
                                                   bool drain)
{
    adc_oneshot_chan_cfg_t channel_config = { .atten = ROTARY_ADC_ATTEN,
                                              .bitwidth = ADC_BITWIDTH_DEFAULT };
    bool ga_pull_up;
    bool ga_pull_down;
    bool bb_pull_up;
    bool bb_pull_down;
    int raw = 0;
    int mv = 0;
    pet_esp32_rotary_ga_class_t result;

    if (backend == NULL || backend->adc_unit == NULL) {
        return PET_ESP32_ROTARY_GA_UNCERTAIN;
    }
    gpio_intr_disable((gpio_num_t)backend->ga_gpio);
    gpio_intr_disable((gpio_num_t)backend->bb_gpio);
    if (drain) {
        drain_raw_queue(backend);
    }
    if (adc_oneshot_config_channel(backend->adc_unit, backend->adc_channel,
                                   &channel_config) != ESP_OK ||
        adc_oneshot_read(backend->adc_unit, backend->adc_channel, &raw) != ESP_OK) {
        result = PET_ESP32_ROTARY_GA_UNCERTAIN;
        mv = 0;
    } else if (backend->adc_calibration_enabled &&
               adc_cali_raw_to_voltage(backend->adc_cali, raw, &mv) == ESP_OK) {
        if (mv < (int)backend->adc_low_max_mv) {
            result = PET_ESP32_ROTARY_GA_LOW;
        } else if (mv >= (int)backend->adc_press_min_mv &&
                   mv <= (int)backend->adc_press_max_mv) {
            result = PET_ESP32_ROTARY_GA_MID;
        } else if (mv > (int)backend->adc_high_min_mv) {
            result = PET_ESP32_ROTARY_GA_HIGH;
        } else {
            result = PET_ESP32_ROTARY_GA_UNCERTAIN;
        }
    } else {
        result = PET_ESP32_ROTARY_GA_UNCERTAIN;
    }

    if (pull_modes(backend->ga_pull, &ga_pull_up, &ga_pull_down) == PET_STATUS_OK) {
        (void)configure_pin(backend->ga_gpio, ga_pull_up, ga_pull_down);
    }
    if (pull_modes(backend->bb_pull, &bb_pull_up, &bb_pull_down) == PET_STATUS_OK) {
        (void)configure_pin(backend->bb_gpio, bb_pull_up, bb_pull_down);
    }
    gpio_intr_enable((gpio_num_t)backend->ga_gpio);
    gpio_intr_enable((gpio_num_t)backend->bb_gpio);
    backend->decoder.previous_ab = read_ab(backend);
    return result;
}

static void push_event(pet_esp32_rotary_t *backend, pet_event_type_t type)
{
    pet_event_t event = { .type = type };
    if (backend->events == NULL) {
        return;
    }
    if (xQueueSend(backend->events, &event, 0U) != pdTRUE) {
        backend->dropped_event_count++;
    }
}

/* Quadrature accumulation: a full detent (+/- transitions_per_detent) emits one
 * NAV event and keeps the leftover transitions instead of clearing them. */
static void feed_quadrature(pet_esp32_rotary_t *backend, uint8_t previous_ab,
                            uint8_t current_ab)
{
    int8_t delta = quadrature_delta(previous_ab, current_ab);
    int16_t accumulator;
    backend->decoder.previous_ab = current_ab;
    if (delta == 0) {
        if (current_ab != previous_ab) {
            backend->decoder.invalid_transition_count++;
        }
        return;
    }
    accumulator = (int16_t)backend->decoder.accumulator + (int16_t)delta;
    if (accumulator >= (int16_t)backend->transitions_per_detent) {
        accumulator -= (int16_t)backend->transitions_per_detent;
        backend->decoder.cw_count++;
        push_event(backend, PET_EVENT_NAV_NEXT);
    } else if (accumulator <= -(int16_t)backend->transitions_per_detent) {
        accumulator += (int16_t)backend->transitions_per_detent;
        backend->decoder.ccw_count++;
        push_event(backend, PET_EVENT_NAV_PREV);
    }
    backend->decoder.accumulator = (int8_t)accumulator;
}

/* Press state machine: MID stable for the debounce window produces exactly one
 * INTERACT; HIGH stable for the debounce window releases. LOW keeps the session
 * pressed while rotating, and the press transitions never reach the quadrature
 * accumulator. */
static void press_fsm_update(pet_esp32_rotary_t *backend,
                             pet_esp32_rotary_ga_class_t class, uint64_t now_us)
{
    uint64_t debounce_us = (uint64_t)backend->press_debounce_ms * 1000ULL;
    switch (backend->press_state) {
    case PET_ESP32_ROTARY_PRESS_CANDIDATE:
        if (class == PET_ESP32_ROTARY_GA_MID ||
            class == PET_ESP32_ROTARY_GA_UNCERTAIN) {
            if (now_us - backend->press_phase_start_us >= debounce_us) {
                backend->press_state = PET_ESP32_ROTARY_PRESS_PRESSED;
                backend->press_count++;
                push_event(backend, PET_EVENT_BUTTON);
            }
        } else {
            backend->press_state = PET_ESP32_ROTARY_PRESS_RELEASED;
        }
        break;
    case PET_ESP32_ROTARY_PRESS_PRESSED:
        if (class == PET_ESP32_ROTARY_GA_HIGH) {
            backend->press_state = PET_ESP32_ROTARY_PRESS_RELEASE_CANDIDATE;
            backend->press_phase_start_us = now_us;
        }
        break;
    case PET_ESP32_ROTARY_PRESS_RELEASE_CANDIDATE:
        if (class == PET_ESP32_ROTARY_GA_HIGH) {
            if (now_us - backend->press_phase_start_us >= debounce_us) {
                backend->press_state = PET_ESP32_ROTARY_PRESS_RELEASED;
            }
        } else {
            backend->press_state = PET_ESP32_ROTARY_PRESS_PRESSED;
        }
        break;
    case PET_ESP32_ROTARY_PRESS_RELEASED:
        break;
    }
}

static void rotary_task(void *arg)
{
    pet_esp32_rotary_t *backend = arg;
    pet_esp32_rotary_edge_t edge;
    BaseType_t received;

    for (;;) {
        uint64_t now_us;
        received = xQueueReceive(backend->raw_edges, &edge, pdMS_TO_TICKS(10U));
        now_us = esp_timer_get_time();

        if (backend->press_state != PET_ESP32_ROTARY_PRESS_RELEASED) {
            if (now_us - backend->last_adc_us >= ROTARY_PRESS_ADC_MIN_US) {
                pet_esp32_rotary_ga_class_t class =
                    sample_ga_class(backend, true);
                backend->last_adc_us = now_us;
                press_fsm_update(backend, class, now_us);
            }
            continue;
        }

        if (received != pdTRUE) {
            continue;
        }

        {
            uint8_t previous_ga = (backend->decoder.previous_ab >> 1U) & 0x1U;
            uint8_t current_ga = (edge.ab >> 1U) & 0x1U;
            if (previous_ga == 1U && current_ga == 0U &&
                now_us - backend->last_adc_us >= ROTARY_EDGE_ADC_MIN_US) {
                uint8_t old_ab = backend->decoder.previous_ab;
                pet_esp32_rotary_ga_class_t class =
                    sample_ga_class(backend, false);
                backend->last_adc_us = now_us;
                if (class == PET_ESP32_ROTARY_GA_MID ||
                    class == PET_ESP32_ROTARY_GA_UNCERTAIN) {
                    backend->press_state =
                        PET_ESP32_ROTARY_PRESS_CANDIDATE;
                    backend->press_phase_start_us = now_us;
                    drain_raw_queue(backend);
                    continue;
                }
                feed_quadrature(backend, old_ab, edge.ab);
            } else {
                feed_quadrature(backend, backend->decoder.previous_ab, edge.ab);
            }
        }
    }
}

void pet_esp32_rotary_decoder_init(pet_esp32_rotary_decoder_t *decoder,
                                   uint8_t initial_ab,
                                   uint8_t transitions_per_detent)
{
    if (decoder == NULL) {
        return;
    }
    *decoder = (pet_esp32_rotary_decoder_t){ .previous_ab = initial_ab & 0x3U,
                                             .transitions_per_detent =
                                                 transitions_per_detent,
                                             .initialized = true };
}

pet_esp32_rotary_direction_t pet_esp32_rotary_decoder_update(
    pet_esp32_rotary_decoder_t *decoder, uint8_t current_ab)
{
    int8_t delta;
    if (decoder == NULL || !decoder->initialized ||
        decoder->transitions_per_detent == 0U) {
        return PET_ESP32_ROTARY_NONE;
    }
    current_ab &= 0x3U;
    delta = quadrature_delta(decoder->previous_ab, current_ab);
    if (delta == 0) {
        if (current_ab != decoder->previous_ab) {
            decoder->invalid_transition_count++;
        }
        decoder->previous_ab = current_ab;
        return PET_ESP32_ROTARY_NONE;
    }

    decoder->previous_ab = current_ab;
    decoder->accumulator = (int8_t)(decoder->accumulator + delta);
    if (decoder->accumulator >= (int8_t)decoder->transitions_per_detent) {
        decoder->accumulator =
            (int8_t)(decoder->accumulator - (int8_t)decoder->transitions_per_detent);
        decoder->cw_count++;
        return PET_ESP32_ROTARY_CW;
    }
    if (decoder->accumulator <= -(int8_t)decoder->transitions_per_detent) {
        decoder->accumulator =
            (int8_t)(decoder->accumulator + (int8_t)decoder->transitions_per_detent);
        decoder->ccw_count++;
        return PET_ESP32_ROTARY_CCW;
    }
    return PET_ESP32_ROTARY_NONE;
}

pet_status_t pet_esp32_rotary_create(pet_esp32_rotary_t *backend,
                                     const pet_esp32_board_config_t *config)
{
    if (backend == NULL || config == NULL) {
        return PET_STATUS_INVALID_ARGUMENT;
    }
    memset(backend, 0, sizeof(*backend));
    backend->ga_gpio = config->rotary_ga_gpio;
    backend->bb_gpio = config->rotary_bb_gpio;
    backend->ga_pull = config->rotary_ga_pull;
    backend->bb_pull = config->rotary_bb_pull;
    backend->transitions_per_detent = config->rotary_transitions_per_detent;
    backend->adc_low_max_mv = config->rotary_adc_low_max_mv;
    backend->adc_press_min_mv = config->rotary_adc_press_min_mv;
    backend->adc_press_max_mv = config->rotary_adc_press_max_mv;
    backend->adc_high_min_mv = config->rotary_adc_high_min_mv;
    backend->press_debounce_ms = config->rotary_press_debounce_ms;
    return PET_STATUS_OK;
}

static pet_status_t init_adc(pet_esp32_rotary_t *backend)
{
    adc_oneshot_unit_init_cfg_t unit_config;
    esp_err_t result;
    result = adc_oneshot_io_to_channel(backend->ga_gpio, &backend->adc_unit_id,
                                       &backend->adc_channel);
    if (result != ESP_OK) {
        ESP_LOGE(TAG, "GA GPIO is not ADC-capable: gpio=%d", backend->ga_gpio);
        return PET_STATUS_INVALID_ARGUMENT;
    }
    unit_config = (adc_oneshot_unit_init_cfg_t){ .unit_id = backend->adc_unit_id,
                                                .clk_src = ADC_RTC_CLK_SRC_DEFAULT,
                                                .ulp_mode = ADC_ULP_MODE_DISABLE };
    if (adc_oneshot_new_unit(&unit_config, &backend->adc_unit) != ESP_OK) {
        return PET_STATUS_IO_ERROR;
    }
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    {
        adc_cali_curve_fitting_config_t calibration_config = {
            .unit_id = backend->adc_unit_id,
            .chan = backend->adc_channel,
            .atten = ROTARY_ADC_ATTEN,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        result = adc_cali_create_scheme_curve_fitting(&calibration_config,
                                                      &backend->adc_cali);
        backend->adc_calibration_enabled = result == ESP_OK;
        ESP_LOGI(TAG, "ADC calibration: curve fitting %s",
                 backend->adc_calibration_enabled ? "enabled" : "unavailable");
    }
#elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    {
        adc_cali_line_fitting_config_t calibration_config = {
            .unit_id = backend->adc_unit_id,
            .atten = ROTARY_ADC_ATTEN,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        result = adc_cali_create_scheme_line_fitting(&calibration_config,
                                                     &backend->adc_cali);
        backend->adc_calibration_enabled = result == ESP_OK;
        ESP_LOGI(TAG, "ADC calibration: line fitting %s",
                 backend->adc_calibration_enabled ? "enabled" : "unavailable");
    }
#else
    backend->adc_calibration_enabled = false;
    ESP_LOGI(TAG, "ADC calibration: unavailable");
#endif
    ESP_LOGI(TAG,
             "GA ADC mapping: GPIO%d -> ADC%d channel %d, atten=ADC_ATTEN_DB_12",
             backend->ga_gpio, backend->adc_unit_id + 1,
             (int)backend->adc_channel);
    return PET_STATUS_OK;
}

pet_status_t pet_esp32_rotary_start(pet_esp32_rotary_t *backend)
{
    bool ga_pull_up;
    bool ga_pull_down;
    bool bb_pull_up;
    bool bb_pull_down;
    pet_status_t status;
    if (backend == NULL) {
        return PET_STATUS_INVALID_ARGUMENT;
    }
    if (!config_is_verified(backend)) {
        ESP_LOGW(TAG,
                 "Rotary input disabled: GPIO mapping not verified "
                 "(GA=%d BB=%d GA_PULL=%s BB_PULL=%s transitions=%u)",
                 backend->ga_gpio, backend->bb_gpio,
                 backend->ga_pull == NULL ? "NULL" : backend->ga_pull,
                 backend->bb_pull == NULL ? "NULL" : backend->bb_pull,
                 (unsigned)backend->transitions_per_detent);
        backend->enabled = false;
        return PET_STATUS_OK;
    }
    if (pull_modes(backend->ga_pull, &ga_pull_up, &ga_pull_down) != PET_STATUS_OK ||
        pull_modes(backend->bb_pull, &bb_pull_up, &bb_pull_down) != PET_STATUS_OK) {
        return PET_STATUS_INVALID_ARGUMENT;
    }

    backend->raw_edges =
        xQueueCreate(ROTARY_RAW_QUEUE_LENGTH, sizeof(pet_esp32_rotary_edge_t));
    backend->events = xQueueCreate(ROTARY_EVENT_QUEUE_LENGTH, sizeof(pet_event_t));
    if (backend->raw_edges == NULL || backend->events == NULL) {
        return PET_STATUS_NO_MEMORY;
    }
    status = init_adc(backend);
    if (status != PET_STATUS_OK) {
        return status;
    }
    if (gpio_install_isr_service(ESP_INTR_FLAG_IRAM) != ESP_OK) {
        return PET_STATUS_IO_ERROR;
    }
    if (gpio_isr_handler_add((gpio_num_t)backend->ga_gpio, rotary_edge_isr,
                             backend) != ESP_OK ||
        gpio_isr_handler_add((gpio_num_t)backend->bb_gpio, rotary_edge_isr,
                             backend) != ESP_OK) {
        return PET_STATUS_IO_ERROR;
    }
    status = configure_pin(backend->ga_gpio, ga_pull_up, ga_pull_down);
    if (status == PET_STATUS_OK) {
        status = configure_pin(backend->bb_gpio, bb_pull_up, bb_pull_down);
    }
    if (status != PET_STATUS_OK) {
        return status;
    }
    pet_esp32_rotary_decoder_init(&backend->decoder, read_ab(backend),
                                  backend->transitions_per_detent);
    backend->press_state = PET_ESP32_ROTARY_PRESS_RELEASED;
    backend->press_phase_start_us = esp_timer_get_time();
    backend->last_adc_us = backend->press_phase_start_us;
    backend->enabled = true;

    if (xTaskCreatePinnedToCore(rotary_task, "rotary", ROTARY_TASK_STACK_WORDS,
                                backend, ROTARY_TASK_PRIORITY, &backend->task,
                                1) != pdPASS) {
        return PET_STATUS_NO_MEMORY;
    }
    ESP_LOGI(TAG,
             "Rotary input enabled: GA=%d BB=%d GA_PULL=%s BB_PULL=%s transitions=%u",
             backend->ga_gpio, backend->bb_gpio, backend->ga_pull,
             backend->bb_pull, (unsigned)backend->transitions_per_detent);
    ESP_LOGI(TAG,
             "Press classifier: LOW<%u mV, MID=%u..%u mV, HIGH>%u mV, debounce=%u ms",
             (unsigned)backend->adc_low_max_mv,
             (unsigned)backend->adc_press_min_mv,
             (unsigned)backend->adc_press_max_mv,
             (unsigned)backend->adc_high_min_mv,
             (unsigned)backend->press_debounce_ms);
    return PET_STATUS_OK;
}

void pet_esp32_rotary_drain_events(pet_esp32_rotary_t *backend, pet_app_t *app)
{
    pet_event_t event;
    uint64_t now_us;
    if (backend == NULL || app == NULL || backend->events == NULL) {
        return;
    }
    while (xQueueReceive(backend->events, &event, 0U) == pdTRUE) {
        if (event.type == PET_EVENT_NAV_NEXT) {
            ESP_LOGI(TAG, "ROTARY CW; EVENT NAV_NEXT");
        } else if (event.type == PET_EVENT_NAV_PREV) {
            ESP_LOGI(TAG, "ROTARY CCW; EVENT NAV_PREV");
        } else if (event.type == PET_EVENT_BUTTON) {
            ESP_LOGI(TAG, "ROTARY PRESS; EVENT INTERACT");
        }
        event.timestamp_ms = app->now_ms;
        if (!pet_app_post_event(app, &event)) {
            ESP_LOGW(TAG, "Shared event queue full; dropped event");
            backend->dropped_event_count++;
        }
    }
    now_us = esp_timer_get_time();
    if (now_us - backend->last_stats_us >= ROTARY_STATS_PERIOD_US) {
        backend->last_stats_us = now_us;
        ESP_LOGI(TAG,
                 "stats: CW=%u CCW=%u press=%u invalid=%u dropped=%u accumulator=%d",
                 (unsigned)backend->decoder.cw_count,
                 (unsigned)backend->decoder.ccw_count,
                 (unsigned)backend->press_count,
                 (unsigned)backend->decoder.invalid_transition_count,
                 (unsigned)backend->dropped_event_count,
                 (int)backend->decoder.accumulator);
    }
}

bool pet_esp32_rotary_is_enabled(const pet_esp32_rotary_t *backend)
{
    return backend != NULL && backend->enabled;
}

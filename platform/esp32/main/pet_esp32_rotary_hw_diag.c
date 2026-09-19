#include "pet_esp32_rotary_hw_diag.h"

#include <stdint.h>
#include <string.h>

#include "driver/gpio.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#define PET_STRINGIFY_INNER(value) #value
#define PET_STRINGIFY(value)        PET_STRINGIFY_INNER(value)

#if !defined(PET_ROTARY_DIAG_MODE) || !defined(PET_ROTARY_HW_DIAG_GA_GPIO) || \
    !defined(PET_ROTARY_HW_DIAG_BB_GPIO)
#error "ESP32 rotary hardware diagnostic build configuration is incomplete"
#endif

#define ROTARY_HW_DIAG_TAG         "rotary_hw"
#define ROTARY_HW_DIAG_EDGE_QUEUE  64U
#define ROTARY_HW_DIAG_SUMMARY_US  1000000ULL
#define ROTARY_HW_DIAG_QUIET_US    400000ULL
#define ROTARY_HW_DIAG_ADC_PERIOD_US 500000ULL
#define ROTARY_HW_DIAG_ADC_RAW_STEP 80
#define ROTARY_HW_DIAG_ADC_MV_STEP  80

typedef struct {
    adc_oneshot_unit_handle_t adc;
    adc_cali_handle_t calibration;
    adc_unit_t adc_unit;
    adc_channel_t adc_channel;
    bool calibration_enabled;
} rotary_hw_diag_adc_t;

typedef struct {
    int raw;
    int mv;
    bool mv_valid;
    int ga_level;
    int bb_level;
    uint8_t ab;
} rotary_hw_diag_sample_t;

typedef struct {
    uint64_t timestamp_us;
    uint8_t ab;
} rotary_hw_diag_edge_t;

static QueueHandle_t s_edge_queue;

static bool diag_mode_is(const char *name)
{
    return strcmp(PET_STRINGIFY(PET_ROTARY_DIAG_MODE), name) == 0;
}

const char *pet_esp32_rotary_diag_mode_name(void)
{
    return PET_STRINGIFY(PET_ROTARY_DIAG_MODE);
}

bool pet_esp32_rotary_diag_mode_is_valid(void)
{
    return diag_mode_is("NONE") || diag_mode_is("ROTARY_HW_DIAG") ||
           diag_mode_is("ADC_MARGIN");
}

bool pet_esp32_rotary_hw_diag_enabled(void)
{
    return diag_mode_is("ROTARY_HW_DIAG");
}

bool pet_esp32_rotary_adc_margin_enabled(void)
{
    return diag_mode_is("ADC_MARGIN");
}

static int absolute_diff(int left, int right)
{
    return left > right ? left - right : right - left;
}

static int8_t quadrature_delta(uint8_t previous_ab, uint8_t current_ab)
{
    static const int8_t transition_table[16] = {
        0,  1, -1, 0,
       -1,  0,  0, 1,
        1,  0,  0, -1,
        0, -1,  1, 0,
    };
    return transition_table[((previous_ab & 0x3U) << 2U) | (current_ab & 0x3U)];
}

static void format_ab(uint8_t ab, char output[3])
{
    output[0] = (ab & 0x2U) != 0U ? '1' : '0';
    output[1] = (ab & 0x1U) != 0U ? '1' : '0';
    output[2] = '\0';
}

static uint8_t read_ab(void)
{
    return (uint8_t)(((gpio_get_level((gpio_num_t)PET_ROTARY_HW_DIAG_GA_GPIO) != 0)
                          ? 0x2U : 0U) |
                     ((gpio_get_level((gpio_num_t)PET_ROTARY_HW_DIAG_BB_GPIO) != 0)
                          ? 0x1U : 0U));
}

static pet_status_t configure_gpio_inputs(void)
{
    gpio_config_t config;
    if (!GPIO_IS_VALID_GPIO(PET_ROTARY_HW_DIAG_GA_GPIO) ||
        !GPIO_IS_VALID_GPIO(PET_ROTARY_HW_DIAG_BB_GPIO)) {
        return PET_STATUS_INVALID_ARGUMENT;
    }
    config = (gpio_config_t){ .pin_bit_mask =
                                  (1ULL << PET_ROTARY_HW_DIAG_GA_GPIO) |
                                  (1ULL << PET_ROTARY_HW_DIAG_BB_GPIO),
                              .mode = GPIO_MODE_INPUT,
                              .pull_up_en = GPIO_PULLUP_DISABLE,
                              .pull_down_en = GPIO_PULLDOWN_DISABLE,
                              .intr_type = GPIO_INTR_ANYEDGE };
    return gpio_config(&config) == ESP_OK ? PET_STATUS_OK : PET_STATUS_IO_ERROR;
}

static void IRAM_ATTR rotary_edge_isr(void *arg)
{
    rotary_hw_diag_edge_t edge;
    BaseType_t wake = pdFALSE;
    (void)arg;
    if (s_edge_queue == NULL) {
        return;
    }
    edge.timestamp_us = (uint64_t)esp_timer_get_time();
    edge.ab = read_ab();
    (void)xQueueSendFromISR(s_edge_queue, &edge, &wake);
    if (wake == pdTRUE) {
        portYIELD_FROM_ISR();
    }
}

static void drain_edge_queue(void)
{
    rotary_hw_diag_edge_t edge;
    while (xQueueReceive(s_edge_queue, &edge, 0U) == pdTRUE) {
    }
}

static pet_status_t init_gpio_isr(void)
{
    s_edge_queue =
        xQueueCreate(ROTARY_HW_DIAG_EDGE_QUEUE, sizeof(rotary_hw_diag_edge_t));
    if (s_edge_queue == NULL) {
        return PET_STATUS_NO_MEMORY;
    }
    if (gpio_install_isr_service(0) != ESP_OK) {
        return PET_STATUS_IO_ERROR;
    }
    if (gpio_isr_handler_add((gpio_num_t)PET_ROTARY_HW_DIAG_GA_GPIO,
                             rotary_edge_isr, NULL) != ESP_OK ||
        gpio_isr_handler_add((gpio_num_t)PET_ROTARY_HW_DIAG_BB_GPIO,
                             rotary_edge_isr, NULL) != ESP_OK) {
        return PET_STATUS_IO_ERROR;
    }
    return configure_gpio_inputs();
}

static pet_status_t init_adc(rotary_hw_diag_adc_t *adc)
{
    adc_oneshot_unit_init_cfg_t unit_config;
    esp_err_t result;
    if (adc == NULL) {
        return PET_STATUS_INVALID_ARGUMENT;
    }
    memset(adc, 0, sizeof(*adc));
    result = adc_oneshot_io_to_channel(PET_ROTARY_HW_DIAG_GA_GPIO,
                                       &adc->adc_unit, &adc->adc_channel);
    if (result != ESP_OK) {
        ESP_LOGE(ROTARY_HW_DIAG_TAG, "GA GPIO is not ADC-capable: gpio=%d",
                 PET_ROTARY_HW_DIAG_GA_GPIO);
        return PET_STATUS_INVALID_ARGUMENT;
    }

    unit_config = (adc_oneshot_unit_init_cfg_t){ .unit_id = adc->adc_unit,
                                                .clk_src = ADC_RTC_CLK_SRC_DEFAULT,
                                                .ulp_mode = ADC_ULP_MODE_DISABLE };
    if (adc_oneshot_new_unit(&unit_config, &adc->adc) != ESP_OK) {
        return PET_STATUS_IO_ERROR;
    }

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    {
        adc_cali_curve_fitting_config_t calibration_config = {
            .unit_id = adc->adc_unit,
            .chan = adc->adc_channel,
            .atten = ADC_ATTEN_DB_12,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        result = adc_cali_create_scheme_curve_fitting(&calibration_config,
                                                      &adc->calibration);
        adc->calibration_enabled = result == ESP_OK;
        ESP_LOGI(ROTARY_HW_DIAG_TAG, "ADC calibration: curve fitting %s",
                 adc->calibration_enabled ? "enabled" : "unavailable");
    }
#elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    {
        adc_cali_line_fitting_config_t calibration_config = {
            .unit_id = adc->adc_unit,
            .atten = ADC_ATTEN_DB_12,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        result = adc_cali_create_scheme_line_fitting(&calibration_config,
                                                     &adc->calibration);
        adc->calibration_enabled = result == ESP_OK;
        ESP_LOGI(ROTARY_HW_DIAG_TAG, "ADC calibration: line fitting %s",
                 adc->calibration_enabled ? "enabled" : "unavailable");
    }
#else
    adc->calibration_enabled = false;
    ESP_LOGI(ROTARY_HW_DIAG_TAG, "ADC calibration: unavailable");
#endif

    ESP_LOGI(ROTARY_HW_DIAG_TAG,
             "GA ADC mapping: GPIO%d -> ADC%d channel %d, atten=ADC_ATTEN_DB_12, bitwidth=default",
             PET_ROTARY_HW_DIAG_GA_GPIO, adc->adc_unit + 1,
             (int)adc->adc_channel);
    return PET_STATUS_OK;
}

/* Pauses digital capture, samples GA voltage through the ADC, then restores
 * the digital input path and re-baselines AB from the current pad levels. */
static pet_status_t sample_ga_adc(rotary_hw_diag_adc_t *adc,
                                  rotary_hw_diag_sample_t *sample)
{
    adc_oneshot_chan_cfg_t channel_config = { .atten = ADC_ATTEN_DB_12,
                                              .bitwidth = ADC_BITWIDTH_DEFAULT };
    if (adc == NULL || sample == NULL || adc->adc == NULL) {
        return PET_STATUS_INVALID_ARGUMENT;
    }
    gpio_intr_disable((gpio_num_t)PET_ROTARY_HW_DIAG_GA_GPIO);
    gpio_intr_disable((gpio_num_t)PET_ROTARY_HW_DIAG_BB_GPIO);
    drain_edge_queue();
    if (adc_oneshot_config_channel(adc->adc, adc->adc_channel,
                                   &channel_config) != ESP_OK) {
        return PET_STATUS_IO_ERROR;
    }
    if (adc_oneshot_read(adc->adc, adc->adc_channel, &sample->raw) != ESP_OK) {
        return PET_STATUS_IO_ERROR;
    }
    sample->mv_valid = adc->calibration_enabled &&
                       adc_cali_raw_to_voltage(adc->calibration, sample->raw,
                                               &sample->mv) == ESP_OK;
    if (configure_gpio_inputs() != PET_STATUS_OK) {
        return PET_STATUS_IO_ERROR;
    }
    gpio_intr_enable((gpio_num_t)PET_ROTARY_HW_DIAG_GA_GPIO);
    gpio_intr_enable((gpio_num_t)PET_ROTARY_HW_DIAG_BB_GPIO);
    drain_edge_queue();
    sample->ga_level = gpio_get_level((gpio_num_t)PET_ROTARY_HW_DIAG_GA_GPIO);
    sample->bb_level = gpio_get_level((gpio_num_t)PET_ROTARY_HW_DIAG_BB_GPIO);
    sample->ab = (uint8_t)((sample->ga_level != 0 ? 0x2U : 0U) |
                           (sample->bb_level != 0 ? 0x1U : 0U));
    return PET_STATUS_OK;
}

static bool adc_changed_significantly(const rotary_hw_diag_sample_t *previous,
                                      const rotary_hw_diag_sample_t *current)
{
    if (previous == NULL || current == NULL) {
        return true;
    }
    if (previous->mv_valid && current->mv_valid) {
        return absolute_diff(previous->mv, current->mv) >= ROTARY_HW_DIAG_ADC_MV_STEP;
    }
    return absolute_diff(previous->raw, current->raw) >= ROTARY_HW_DIAG_ADC_RAW_STEP;
}

static void log_sample(const char *prefix, const rotary_hw_diag_sample_t *sample,
                       uint32_t transition_count, uint32_t invalid_count)
{
    char ab[3];
    format_ab(sample->ab, ab);
    if (sample->mv_valid) {
        ESP_LOGI(ROTARY_HW_DIAG_TAG,
                 "%s GA raw=%d, GA=%d mV, GA=%d, BB=%d, AB=%s, transitions=%u, invalid=%u",
                 prefix, sample->raw, sample->mv, sample->ga_level, sample->bb_level,
                 ab, (unsigned)transition_count, (unsigned)invalid_count);
    } else {
        ESP_LOGI(ROTARY_HW_DIAG_TAG,
                 "%s GA raw=%d, GA=mV unavailable, GA=%d, BB=%d, AB=%s, transitions=%u, invalid=%u",
                 prefix, sample->raw, sample->ga_level, sample->bb_level, ab,
                 (unsigned)transition_count, (unsigned)invalid_count);
    }
}

pet_status_t pet_esp32_rotary_hw_diag_run(void)
{
    rotary_hw_diag_adc_t adc;
    rotary_hw_diag_sample_t adc_sample;
    rotary_hw_diag_sample_t previous_adc;
    uint8_t previous_ab;
    bool have_previous = false;
    uint64_t previous_edge_us = 0U;
    uint64_t last_edge_us;
    uint64_t last_adc_us;
    uint64_t last_summary_us;
    uint32_t transition_count = 0U;
    uint32_t invalid_transition_count = 0U;
    pet_status_t status;

    ESP_LOGI(ROTARY_HW_DIAG_TAG, "Rotary HW diagnostic");
    ESP_LOGI(ROTARY_HW_DIAG_TAG, "GA GPIO: %d", PET_ROTARY_HW_DIAG_GA_GPIO);
    ESP_LOGI(ROTARY_HW_DIAG_TAG, "BB GPIO: %d", PET_ROTARY_HW_DIAG_BB_GPIO);
    ESP_LOGI(ROTARY_HW_DIAG_TAG, "VCC: 3.3V external module supply");
    ESP_LOGI(ROTARY_HW_DIAG_TAG, "Internal pulls: disabled on GA and BB");
    ESP_LOGI(ROTARY_HW_DIAG_TAG,
             "Capture: GPIO ISR any-edge on GA/BB, edge queue depth=%u",
             (unsigned)ROTARY_HW_DIAG_EDGE_QUEUE);
    ESP_LOGI(ROTARY_HW_DIAG_TAG,
             "ADC: GA sampled every %u ms when AB quiet for %u ms",
             (unsigned)(ROTARY_HW_DIAG_ADC_PERIOD_US / 1000ULL),
             (unsigned)(ROTARY_HW_DIAG_QUIET_US / 1000ULL));

    status = init_gpio_isr();
    if (status != PET_STATUS_OK) {
        return status;
    }
    status = init_adc(&adc);
    if (status != PET_STATUS_OK) {
        return status;
    }
    status = sample_ga_adc(&adc, &adc_sample);
    if (status != PET_STATUS_OK) {
        return status;
    }
    previous_adc = adc_sample;
    previous_ab = adc_sample.ab;
    have_previous = true;
    log_sample("initial:", &adc_sample, transition_count,
               invalid_transition_count);
    last_edge_us = esp_timer_get_time();
    last_adc_us = last_edge_us;
    last_summary_us = last_edge_us;

    for (;;) {
        rotary_hw_diag_edge_t edge;
        TickType_t wait_ticks = pdMS_TO_TICKS(20U);
        while (xQueueReceive(s_edge_queue, &edge, wait_ticks) == pdTRUE) {
            char previous_name[3];
            char current_name[3];
            int8_t delta;
            wait_ticks = 0U;
            if (!have_previous) {
                previous_ab = edge.ab;
                have_previous = true;
                previous_edge_us = edge.timestamp_us;
                continue;
            }
            if (edge.ab == previous_ab) {
                continue;
            }
            delta = quadrature_delta(previous_ab, edge.ab);
            if (delta == 0) {
                invalid_transition_count++;
            } else {
                transition_count++;
            }
            format_ab(previous_ab, previous_name);
            format_ab(edge.ab, current_name);
            ESP_LOGI(ROTARY_HW_DIAG_TAG,
                     "AB: %s -> %s delta=%d t=%llu us dt=%llu us",
                     previous_name, current_name, (int)delta,
                     (unsigned long long)edge.timestamp_us,
                     (unsigned long long)(previous_edge_us != 0U &&
                                          edge.timestamp_us > previous_edge_us
                                              ? edge.timestamp_us - previous_edge_us
                                              : 0U));
            previous_ab = edge.ab;
            previous_edge_us = edge.timestamp_us;
            last_edge_us = edge.timestamp_us;
        }

        {
            uint64_t now_us = esp_timer_get_time();
            if (now_us - last_edge_us > ROTARY_HW_DIAG_QUIET_US &&
                now_us - last_adc_us >= ROTARY_HW_DIAG_ADC_PERIOD_US) {
                status = sample_ga_adc(&adc, &adc_sample);
                if (status != PET_STATUS_OK) {
                    return status;
                }
                last_adc_us = now_us;
                previous_ab = adc_sample.ab;
                previous_edge_us = 0U;
                if (adc_changed_significantly(&previous_adc, &adc_sample)) {
                    log_sample("adc:", &adc_sample, transition_count,
                               invalid_transition_count);
                    previous_adc = adc_sample;
                }
            }
            if (now_us - last_summary_us >= ROTARY_HW_DIAG_SUMMARY_US) {
                last_summary_us = now_us;
                log_sample("summary:", &adc_sample, transition_count,
                           invalid_transition_count);
            }
        }
    }
}

typedef enum {
    MARGIN_CLASS_LOW = 0,
    MARGIN_CLASS_MID,
    MARGIN_CLASS_HIGH,
    MARGIN_CLASS_UNCERTAIN,
    MARGIN_CLASS_COUNT
} margin_class_t;

typedef struct {
    uint32_t count;
    int min_mv;
    int max_mv;
    int64_t sum_mv;
} margin_stats_t;

static const char *margin_class_name(margin_class_t class)
{
    switch (class) {
    case MARGIN_CLASS_LOW:
        return "LOW";
    case MARGIN_CLASS_MID:
        return "MID";
    case MARGIN_CLASS_HIGH:
        return "HIGH";
    default:
        return "UNCERTAIN";
    }
}

static void margin_stats_reset(margin_stats_t *stats)
{
    stats->count = 0U;
    stats->min_mv = 0;
    stats->max_mv = 0;
    stats->sum_mv = 0;
}

static void margin_stats_add(margin_stats_t *stats, int mv)
{
    if (stats->count == 0U) {
        stats->min_mv = mv;
        stats->max_mv = mv;
    } else {
        if (mv < stats->min_mv) {
            stats->min_mv = mv;
        }
        if (mv > stats->max_mv) {
            stats->max_mv = mv;
        }
    }
    stats->sum_mv += mv;
    stats->count++;
}

static void margin_stats_log(const char *prefix, margin_class_t class,
                             const margin_stats_t *stats)
{
    if (stats->count == 0U) {
        ESP_LOGI(ROTARY_HW_DIAG_TAG, "ADC margin %s [%s]: no samples", prefix,
                 margin_class_name(class));
        return;
    }
    ESP_LOGI(ROTARY_HW_DIAG_TAG,
             "ADC margin %s [%s]: n=%u min=%d max=%d avg=%lld mV", prefix,
             margin_class_name(class), (unsigned)stats->count, stats->min_mv,
             stats->max_mv,
             (long long)(stats->sum_mv / (int64_t)stats->count));
}

static margin_class_t margin_classify(int mv)
{
    if (mv < (int)PET_ROTARY_ADC_LOW_MAX_MV) {
        return MARGIN_CLASS_LOW;
    }
    if (mv >= (int)PET_ROTARY_ADC_PRESS_MIN_MV &&
        mv <= (int)PET_ROTARY_ADC_PRESS_MAX_MV) {
        return MARGIN_CLASS_MID;
    }
    if (mv > (int)PET_ROTARY_ADC_HIGH_MIN_MV) {
        return MARGIN_CLASS_HIGH;
    }
    return MARGIN_CLASS_UNCERTAIN;
}

pet_status_t pet_esp32_rotary_adc_margin_run(void)
{
    rotary_hw_diag_adc_t adc;
    adc_oneshot_chan_cfg_t channel_config = { .atten = ADC_ATTEN_DB_12,
                                              .bitwidth = ADC_BITWIDTH_DEFAULT };
    margin_stats_t stats[MARGIN_CLASS_COUNT];
    margin_class_t current = MARGIN_CLASS_UNCERTAIN;
    bool have_current = false;
    uint64_t last_summary_us;
    pet_status_t status;
    size_t index;

    for (index = 0; index < MARGIN_CLASS_COUNT; index++) {
        margin_stats_reset(&stats[index]);
    }

    ESP_LOGI(ROTARY_HW_DIAG_TAG, "Rotary ADC margin diagnostic");
    ESP_LOGI(ROTARY_HW_DIAG_TAG,
             "Thresholds: LOW<%u mV, MID=%u..%u mV, HIGH>%u mV",
             (unsigned)PET_ROTARY_ADC_LOW_MAX_MV,
             (unsigned)PET_ROTARY_ADC_PRESS_MIN_MV,
             (unsigned)PET_ROTARY_ADC_PRESS_MAX_MV,
             (unsigned)PET_ROTARY_ADC_HIGH_MIN_MV);
    ESP_LOGI(ROTARY_HW_DIAG_TAG,
             "Actions: idle for HIGH, hold press for MID, rotate slowly for LOW");
    ESP_LOGI(ROTARY_HW_DIAG_TAG, "Sampling every 10 ms; stats on class change and 5 s");

    status = init_adc(&adc);
    if (status != PET_STATUS_OK) {
        return status;
    }
    if (adc_oneshot_config_channel(adc.adc, adc.adc_channel,
                                   &channel_config) != ESP_OK) {
        return PET_STATUS_IO_ERROR;
    }
    last_summary_us = esp_timer_get_time();

    for (;;) {
        int raw = 0;
        int mv = 0;
        uint64_t now_us;
        margin_class_t class;
        if (adc_oneshot_read(adc.adc, adc.adc_channel, &raw) != ESP_OK) {
            return PET_STATUS_IO_ERROR;
        }
        if (adc.calibration_enabled &&
            adc_cali_raw_to_voltage(adc.calibration, raw, &mv) != ESP_OK) {
            class = MARGIN_CLASS_UNCERTAIN;
        } else {
            class = margin_classify(mv);
        }
        if (have_current && class != current) {
            margin_stats_log("class done:", current, &stats[current]);
            margin_stats_reset(&stats[current]);
        }
        current = class;
        have_current = true;
        margin_stats_add(&stats[current], mv);

        now_us = esp_timer_get_time();
        if (now_us - last_summary_us >= 5000000ULL) {
            last_summary_us = now_us;
            margin_stats_log("running:", current, &stats[current]);
        }
        vTaskDelay(pdMS_TO_TICKS(10U));
    }
}

#include "pet_esp32_backlight.h"

#include "driver/gpio.h"
#include "driver/ledc.h"

#define BACKLIGHT_LEDC_MODE  LEDC_LOW_SPEED_MODE
#define BACKLIGHT_LEDC_TIMER LEDC_TIMER_0
#define BACKLIGHT_LEDC_CH    LEDC_CHANNEL_0

static uint32_t max_duty(const pet_esp32_backlight_t *backend)
{
    return (1UL << backend->pwm_resolution_bits) - 1UL;
}

static uint32_t percent_to_duty(const pet_esp32_backlight_t *backend, uint8_t percent)
{
    return (max_duty(backend) * percent) / 100U;
}

static pet_status_t set_level(pet_esp32_backlight_t *backend, uint8_t percent)
{
    if (backend == NULL || percent > 100U) {
        return PET_STATUS_INVALID_ARGUMENT;
    }
    if (ledc_set_duty(BACKLIGHT_LEDC_MODE, BACKLIGHT_LEDC_CH,
                      percent_to_duty(backend, percent)) != ESP_OK ||
        ledc_update_duty(BACKLIGHT_LEDC_MODE, BACKLIGHT_LEDC_CH) != ESP_OK) {
        return PET_STATUS_IO_ERROR;
    }
    backend->brightness = percent;
    return PET_STATUS_OK;
}

static pet_status_t backlight_init(void *context)
{
    pet_esp32_backlight_t *backend = context;
    ledc_timer_config_t timer_config;
    ledc_channel_config_t channel_config;
    if (backend == NULL || backend->pwm_frequency_hz == 0U ||
        backend->pwm_resolution_bits == 0U || backend->pwm_resolution_bits > 15U ||
        backend->default_percent > 100U || backend->sleep_percent > 100U) {
        return PET_STATUS_INVALID_ARGUMENT;
    }
    timer_config = (ledc_timer_config_t){ .speed_mode = BACKLIGHT_LEDC_MODE,
                                          .duty_resolution =
                                              (ledc_timer_bit_t)backend->pwm_resolution_bits,
                                          .timer_num = BACKLIGHT_LEDC_TIMER,
                                          .freq_hz = backend->pwm_frequency_hz,
                                          .clk_cfg = LEDC_AUTO_CLK };
    channel_config = (ledc_channel_config_t){ .gpio_num = backend->gpio,
                                             .speed_mode = BACKLIGHT_LEDC_MODE,
                                             .channel = BACKLIGHT_LEDC_CH,
                                             .timer_sel = BACKLIGHT_LEDC_TIMER,
                                             .duty = 0U,
                                             .hpoint = 0,
                                             .sleep_mode =
                                                 LEDC_SLEEP_MODE_NO_ALIVE_NO_PD,
                                             .flags.output_invert =
                                                 backend->active_high ? 0U : 1U };
    if (ledc_timer_config(&timer_config) != ESP_OK ||
        ledc_channel_config(&channel_config) != ESP_OK) {
        return PET_STATUS_IO_ERROR;
    }
    backend->brightness_before_sleep = backend->default_percent;
    return set_level(backend, 0U);
}

static pet_status_t backlight_set_brightness(void *context, uint8_t percent)
{
    return set_level(context, percent);
}

static pet_status_t backlight_fade(void *context, uint8_t percent, uint32_t duration_ms)
{
    (void)context;
    (void)percent;
    (void)duration_ms;
    /* Hardware fade can be added after PWM parameters are verified on target. */
    return PET_STATUS_NOT_SUPPORTED;
}

static pet_status_t backlight_sleep(void *context)
{
    pet_esp32_backlight_t *backend = context;
    if (backend == NULL) {
        return PET_STATUS_INVALID_ARGUMENT;
    }
    backend->brightness_before_sleep = backend->brightness;
    return set_level(backend, backend->sleep_percent);
}

static pet_status_t backlight_wake(void *context)
{
    pet_esp32_backlight_t *backend = context;
    return backend == NULL ? PET_STATUS_INVALID_ARGUMENT
                           : set_level(backend, backend->brightness_before_sleep);
}

static const pet_backlight_ops_t backlight_ops = {
    backlight_init,
    backlight_set_brightness,
    backlight_fade,
    backlight_sleep,
    backlight_wake,
};

pet_status_t pet_esp32_backlight_create(pet_esp32_backlight_t *backend,
                                         pet_backlight_t *backlight, int gpio,
                                         bool active_high,
                                         uint32_t pwm_frequency_hz,
                                         uint8_t pwm_resolution_bits,
                                         uint8_t default_percent,
                                         uint8_t sleep_percent)
{
    if (backend == NULL || backlight == NULL || !GPIO_IS_VALID_OUTPUT_GPIO(gpio) ||
        default_percent > 100U || sleep_percent > 100U) {
        return PET_STATUS_INVALID_ARGUMENT;
    }
    *backend = (pet_esp32_backlight_t){ .gpio = gpio,
                                        .active_high = active_high,
                                        .pwm_frequency_hz = pwm_frequency_hz,
                                        .pwm_resolution_bits = pwm_resolution_bits,
                                        .default_percent = default_percent,
                                        .sleep_percent = sleep_percent };
    backlight->context = backend;
    backlight->ops = &backlight_ops;
    return PET_STATUS_OK;
}

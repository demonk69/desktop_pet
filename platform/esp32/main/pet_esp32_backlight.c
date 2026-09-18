#include "pet_esp32_backlight.h"

#include "driver/gpio.h"

static pet_status_t set_level(pet_esp32_backlight_t *backend, uint8_t percent)
{
    int enabled;
    if (backend == NULL || percent > 100U) {
        return PET_STATUS_INVALID_ARGUMENT;
    }
    enabled = percent > 0U;
    if (!backend->active_high) {
        enabled = !enabled;
    }
    if (gpio_set_level((gpio_num_t)backend->gpio, enabled) != ESP_OK) {
        return PET_STATUS_IO_ERROR;
    }
    backend->brightness = percent;
    return PET_STATUS_OK;
}

static pet_status_t backlight_init(void *context)
{
    pet_esp32_backlight_t *backend = context;
    gpio_config_t config;
    if (backend == NULL || !GPIO_IS_VALID_OUTPUT_GPIO(backend->gpio)) {
        return PET_STATUS_INVALID_ARGUMENT;
    }
    config = (gpio_config_t){ .pin_bit_mask = 1ULL << backend->gpio,
                              .mode = GPIO_MODE_OUTPUT,
                              .pull_up_en = GPIO_PULLUP_DISABLE,
                              .pull_down_en = GPIO_PULLDOWN_DISABLE,
                              .intr_type = GPIO_INTR_DISABLE };
    if (gpio_config(&config) != ESP_OK) {
        return PET_STATUS_IO_ERROR;
    }
    backend->brightness_before_sleep = 100U;
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
    /* TODO: add LEDC only when variable brightness is required. */
    return PET_STATUS_NOT_SUPPORTED;
}

static pet_status_t backlight_sleep(void *context)
{
    pet_esp32_backlight_t *backend = context;
    if (backend == NULL) {
        return PET_STATUS_INVALID_ARGUMENT;
    }
    backend->brightness_before_sleep = backend->brightness;
    return set_level(backend, 0U);
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
                                        bool active_high)
{
    if (backend == NULL || backlight == NULL || !GPIO_IS_VALID_OUTPUT_GPIO(gpio)) {
        return PET_STATUS_INVALID_ARGUMENT;
    }
    *backend = (pet_esp32_backlight_t){ .gpio = gpio, .active_high = active_high };
    backlight->context = backend;
    backlight->ops = &backlight_ops;
    return PET_STATUS_OK;
}

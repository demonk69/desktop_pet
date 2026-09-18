#include "pet_esp32_display.h"

#include <string.h>

#include "driver/gpio.h"
#include "esp_heap_caps.h"
#include "esp_psram.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "soc/soc_caps.h"

#define ST7789_CMD_SWRESET 0x01U
#define ST7789_CMD_SLPOUT  0x11U
#define ST7789_CMD_NORON   0x13U
#define ST7789_CMD_INVOFF  0x20U
#define ST7789_CMD_INVON   0x21U
#define ST7789_CMD_DISPON  0x29U
#define ST7789_CMD_CASET   0x2AU
#define ST7789_CMD_RASET   0x2BU
#define ST7789_CMD_RAMWR   0x2CU
#define ST7789_CMD_MADCTL  0x36U
#define ST7789_CMD_COLMOD  0x3AU

static pet_status_t transmit(pet_esp32_display_t *backend, const void *data,
                             size_t length)
{
    spi_transaction_t transaction = {
        .length = length * 8U,
        .tx_buffer = data,
    };
    return spi_device_polling_transmit(backend->spi, &transaction) == ESP_OK
               ? PET_STATUS_OK
               : PET_STATUS_IO_ERROR;
}

static pet_status_t send_command(pet_esp32_display_t *backend, uint8_t command)
{
    if (gpio_set_level((gpio_num_t)backend->config.pet.hardware.gpio_dc, 0) != ESP_OK) {
        return PET_STATUS_IO_ERROR;
    }
    return transmit(backend, &command, sizeof(command));
}

static pet_status_t send_data(pet_esp32_display_t *backend, const void *data,
                              size_t length)
{
    if (length == 0U) {
        return PET_STATUS_OK;
    }
    if (gpio_set_level((gpio_num_t)backend->config.pet.hardware.gpio_dc, 1) != ESP_OK) {
        return PET_STATUS_IO_ERROR;
    }
    return transmit(backend, data, length);
}

static pet_status_t send_command_data(pet_esp32_display_t *backend, uint8_t command,
                                      const void *data, size_t length)
{
    pet_status_t status = send_command(backend, command);
    return status == PET_STATUS_OK ? send_data(backend, data, length) : status;
}

static pet_status_t set_window(pet_esp32_display_t *backend, uint16_t x0, uint16_t y0,
                               uint16_t x1, uint16_t y1)
{
    const pet_hardware_config_t *config = &backend->config.pet.hardware;
    uint16_t physical_x0;
    uint16_t physical_x1;
    uint16_t physical_y0;
    uint16_t physical_y1;
    uint8_t columns[4];
    uint8_t rows[4];
    pet_status_t status;

    if (x0 > x1 || y0 > y1 || x1 >= config->width || y1 >= config->height) {
        return PET_STATUS_INVALID_ARGUMENT;
    }
    physical_x0 = (uint16_t)(x0 + config->x_offset);
    physical_x1 = (uint16_t)(x1 + config->x_offset);
    physical_y0 = (uint16_t)(y0 + config->y_offset);
    physical_y1 = (uint16_t)(y1 + config->y_offset);
    columns[0] = (uint8_t)(physical_x0 >> 8U);
    columns[1] = (uint8_t)physical_x0;
    columns[2] = (uint8_t)(physical_x1 >> 8U);
    columns[3] = (uint8_t)physical_x1;
    rows[0] = (uint8_t)(physical_y0 >> 8U);
    rows[1] = (uint8_t)physical_y0;
    rows[2] = (uint8_t)(physical_y1 >> 8U);
    rows[3] = (uint8_t)physical_y1;

    status = send_command_data(backend, ST7789_CMD_CASET, columns, sizeof(columns));
    if (status == PET_STATUS_OK) {
        status = send_command_data(backend, ST7789_CMD_RASET, rows, sizeof(rows));
    }
    return status == PET_STATUS_OK ? send_command(backend, ST7789_CMD_RAMWR) : status;
}

static pet_status_t configure_control_gpio(pet_esp32_display_t *backend)
{
    const pet_hardware_config_t *config = &backend->config.pet.hardware;
    gpio_config_t output_config;
    if (!GPIO_IS_VALID_OUTPUT_GPIO(config->gpio_reset) ||
        !GPIO_IS_VALID_OUTPUT_GPIO(config->gpio_dc)) {
        return PET_STATUS_INVALID_ARGUMENT;
    }
    output_config = (gpio_config_t){
        .pin_bit_mask = (1ULL << config->gpio_reset) | (1ULL << config->gpio_dc),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    if (gpio_config(&output_config) != ESP_OK ||
        gpio_set_level((gpio_num_t)config->gpio_dc, 0) != ESP_OK ||
        gpio_set_level((gpio_num_t)config->gpio_reset, 0) != ESP_OK) {
        return PET_STATUS_IO_ERROR;
    }
    vTaskDelay(pdMS_TO_TICKS(backend->config.reset_low_ms));
    if (gpio_set_level((gpio_num_t)config->gpio_reset, 1) != ESP_OK) {
        return PET_STATUS_IO_ERROR;
    }
    vTaskDelay(pdMS_TO_TICKS(backend->config.reset_high_ms));
    return PET_STATUS_OK;
}

static pet_status_t initialize_spi(pet_esp32_display_t *backend)
{
    const pet_hardware_config_t *config = &backend->config.pet.hardware;
    spi_bus_config_t bus_config = {
        .mosi_io_num = config->gpio_mosi,
        .miso_io_num = -1,
        .sclk_io_num = config->gpio_sclk,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = (int)backend->config.staging_buffer_bytes,
    };
    spi_device_interface_config_t device_config = {
        .clock_speed_hz = (int)config->spi_frequency_hz,
        .mode = backend->config.lcd_spi_mode,
        .spics_io_num = config->gpio_cs,
        .queue_size = 1,
    };
    if (spi_bus_initialize(backend->config.lcd_spi_host, &bus_config,
                           backend->config.lcd_dma_enabled ? SPI_DMA_CH_AUTO
                                                           : SPI_DMA_DISABLED) != ESP_OK) {
        return PET_STATUS_IO_ERROR;
    }
    backend->bus_initialized = true;
    if (spi_bus_add_device(backend->config.lcd_spi_host, &device_config,
                           &backend->spi) != ESP_OK) {
        return PET_STATUS_IO_ERROR;
    }
    {
        int frequency_khz;
        if (spi_device_get_actual_freq(backend->spi, &frequency_khz) != ESP_OK) {
            return PET_STATUS_IO_ERROR;
        }
        backend->actual_spi_frequency_hz = (uint32_t)frequency_khz * 1000U;
    }
    return PET_STATUS_OK;
}

static void release_resources(pet_esp32_display_t *backend)
{
    if (backend->spi != NULL) {
        (void)spi_bus_remove_device(backend->spi);
        backend->spi = NULL;
    }
    if (backend->bus_initialized) {
        (void)spi_bus_free(backend->config.lcd_spi_host);
        backend->bus_initialized = false;
    }
    heap_caps_free(backend->transfer_buffer);
    heap_caps_free(backend->framebuffer);
    backend->transfer_buffer = NULL;
    backend->framebuffer = NULL;
    backend->initialized = false;
}

static pet_status_t initialize_panel(pet_esp32_display_t *backend)
{
    uint8_t pixel_format = 0x55U;
    uint8_t madctl = backend->config.lcd_madctl;
    pet_status_t status = send_command(backend, ST7789_CMD_SWRESET);
    if (status != PET_STATUS_OK) {
        return status;
    }
    vTaskDelay(pdMS_TO_TICKS(150U));
    status = send_command(backend, ST7789_CMD_SLPOUT);
    if (status != PET_STATUS_OK) {
        return status;
    }
    vTaskDelay(pdMS_TO_TICKS(120U));
    status = send_command_data(backend, ST7789_CMD_COLMOD, &pixel_format, 1U);
    if (status == PET_STATUS_OK) {
        status = send_command_data(backend, ST7789_CMD_MADCTL, &madctl, 1U);
    }
    if (status == PET_STATUS_OK) {
        const pet_hardware_config_t *config = &backend->config.pet.hardware;
        status = set_window(backend, 0U, 0U, (uint16_t)(config->width - 1U),
                            (uint16_t)(config->height - 1U));
    }
    if (status == PET_STATUS_OK) {
        status = send_command(backend, backend->config.lcd_inverted ? ST7789_CMD_INVON
                                                                    : ST7789_CMD_INVOFF);
    }
    if (status == PET_STATUS_OK) {
        status = send_command(backend, ST7789_CMD_NORON);
    }
    if (status != PET_STATUS_OK) {
        return status;
    }
    vTaskDelay(pdMS_TO_TICKS(10U));
    status = send_command(backend, ST7789_CMD_DISPON);
    if (status == PET_STATUS_OK) {
        vTaskDelay(pdMS_TO_TICKS(100U));
    }
    return status;
}

static pet_status_t display_init(void *context)
{
    pet_esp32_display_t *backend = context;
    size_t transfer_bytes;
    pet_status_t status;
    if (backend == NULL || backend->config.pet.hardware.bus != PET_LCD_BUS_SPI ||
        backend->config.pet.hardware.rotation != 0U ||
        backend->config.pet.hardware.color_order != PET_LCD_COLOR_RGB ||
        backend->config.staging_buffer_bytes < sizeof(uint16_t) ||
        backend->config.staging_buffer_bytes % sizeof(uint16_t) != 0U ||
        (!backend->config.lcd_dma_enabled &&
         backend->config.staging_buffer_bytes > SOC_SPI_MAXIMUM_BUFFER_SIZE) ||
        !esp_psram_is_initialized()) {
        return PET_STATUS_INVALID_ARGUMENT;
    }
    backend->framebuffer = heap_caps_calloc(
        1U, backend->config.pet.hardware.framebuffer_bytes,
        MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    transfer_bytes = backend->config.staging_buffer_bytes;
    backend->transfer_buffer = heap_caps_malloc(
        transfer_bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT |
                            (backend->config.lcd_dma_enabled ? MALLOC_CAP_DMA : 0U));
    if (backend->framebuffer == NULL || backend->transfer_buffer == NULL) {
        release_resources(backend);
        return PET_STATUS_NO_MEMORY;
    }
    status = configure_control_gpio(backend);
    if (status == PET_STATUS_OK) {
        status = initialize_spi(backend);
    }
    if (status == PET_STATUS_OK) {
        status = initialize_panel(backend);
    }
    if (status != PET_STATUS_OK) {
        release_resources(backend);
        return status;
    }
    backend->initialized = status == PET_STATUS_OK;
    return status;
}

static pet_status_t display_clear(void *context, uint16_t color)
{
    pet_esp32_display_t *backend = context;
    size_t index;
    size_t count;
    if (backend == NULL || backend->framebuffer == NULL) {
        return PET_STATUS_NOT_INITIALIZED;
    }
    count = (size_t)backend->config.pet.hardware.width *
            backend->config.pet.hardware.height;
    for (index = 0U; index < count; index++) {
        backend->framebuffer[index] = color;
    }
    return PET_STATUS_OK;
}

static pet_status_t display_pixel(void *context, int x, int y, uint16_t color)
{
    pet_esp32_display_t *backend = context;
    const pet_hardware_config_t *config;
    if (backend == NULL || backend->framebuffer == NULL) {
        return PET_STATUS_NOT_INITIALIZED;
    }
    config = &backend->config.pet.hardware;
    if (x >= 0 && y >= 0 && x < config->width && y < config->height) {
        backend->framebuffer[(size_t)y * config->width + (size_t)x] = color;
    }
    return PET_STATUS_OK;
}

static pet_status_t display_region(void *context, int x, int y, int width, int height,
                                   const uint16_t *pixels, size_t stride)
{
    int row;
    int column;
    if (pixels == NULL) {
        return PET_STATUS_INVALID_ARGUMENT;
    }
    for (row = 0; row < height; row++) {
        for (column = 0; column < width; column++) {
            pet_status_t status = display_pixel(
                context, x + column, y + row,
                pixels[(size_t)row * stride + (size_t)column]);
            if (status != PET_STATUS_OK) {
                return status;
            }
        }
    }
    return PET_STATUS_OK;
}

static pet_status_t display_bitmap(void *context, int x, int y, int width, int height,
                                   const uint16_t *pixels)
{
    return display_region(context, x, y, width, height, pixels, (size_t)width);
}

static pet_status_t display_flush(void *context)
{
    pet_esp32_display_t *backend = context;
    const pet_hardware_config_t *config;
    size_t pixel_count;
    size_t offset = 0U;
    int64_t start_us;
    pet_status_t status;
    size_t staging_pixels;
    if (backend == NULL || !backend->initialized || backend->framebuffer == NULL ||
        backend->transfer_buffer == NULL) {
        return PET_STATUS_NOT_INITIALIZED;
    }
    start_us = esp_timer_get_time();
    config = &backend->config.pet.hardware;
    staging_pixels = backend->config.staging_buffer_bytes / sizeof(uint16_t);
    if (backend->config.block_height > 0U) {
        staging_pixels = (size_t)config->width * backend->config.block_height;
    }
    status = set_window(backend, 0U, 0U, (uint16_t)(config->width - 1U),
                        (uint16_t)(config->height - 1U));
    if (status != PET_STATUS_OK) {
        return status;
    }
    pixel_count = (size_t)config->width * config->height;
    while (offset < pixel_count) {
        size_t index;
        size_t chunk = pixel_count - offset;
        if (chunk > staging_pixels) {
            chunk = staging_pixels;
        }
        for (index = 0U; index < chunk; index++) {
            uint16_t pixel = backend->framebuffer[offset + index];
            backend->transfer_buffer[index * 2U] = (uint8_t)(pixel >> 8U);
            backend->transfer_buffer[index * 2U + 1U] = (uint8_t)pixel;
        }
        status = send_data(backend, backend->transfer_buffer, chunk * 2U);
        if (status != PET_STATUS_OK) {
            backend->last_flush_us = (uint64_t)(esp_timer_get_time() - start_us);
            return status;
        }
        offset += chunk;
    }
    backend->last_flush_us = (uint64_t)(esp_timer_get_time() - start_us);
    return PET_STATUS_OK;
}

static pet_status_t display_rotation(void *context, uint8_t rotation)
{
    pet_esp32_display_t *backend = context;
    if (backend == NULL || rotation != backend->config.pet.hardware.rotation) {
        return PET_STATUS_NOT_SUPPORTED;
    }
    return send_command_data(backend, ST7789_CMD_MADCTL,
                             &backend->config.lcd_madctl, 1U);
}

static int display_width(void *context)
{
    pet_esp32_display_t *backend = context;
    return backend == NULL ? 0 : backend->config.pet.hardware.width;
}

static int display_height(void *context)
{
    pet_esp32_display_t *backend = context;
    return backend == NULL ? 0 : backend->config.pet.hardware.height;
}

static pet_status_t display_brightness(void *context, uint8_t percent)
{
    (void)context;
    (void)percent;
    return PET_STATUS_NOT_SUPPORTED;
}

static const pet_display_ops_t display_ops = {
    display_init,
    display_clear,
    display_pixel,
    display_bitmap,
    display_region,
    display_flush,
    display_rotation,
    display_width,
    display_height,
    display_brightness,
};

pet_status_t pet_esp32_display_create(pet_esp32_display_t *backend,
                                      pet_display_t *display,
                                      const pet_esp32_board_config_t *config)
{
    if (backend == NULL || display == NULL || config == NULL ||
        pet_config_validate(&config->pet) != PET_STATUS_OK ||
        config->lcd_spi_host != SPI2_HOST || config->lcd_spi_mode > 3U ||
        config->pet.hardware.spi_frequency_hz == 0U ||
        config->staging_buffer_bytes < sizeof(uint16_t) ||
        config->staging_buffer_bytes % sizeof(uint16_t) != 0U ||
        (!config->lcd_dma_enabled &&
         config->staging_buffer_bytes > SOC_SPI_MAXIMUM_BUFFER_SIZE) ||
        config->block_height > config->pet.hardware.height ||
        (config->block_height > 0U &&
         config->staging_buffer_bytes < (size_t)config->pet.hardware.width *
                                            config->block_height * sizeof(uint16_t)) ||
        !GPIO_IS_VALID_OUTPUT_GPIO(config->pet.hardware.gpio_mosi) ||
        !GPIO_IS_VALID_OUTPUT_GPIO(config->pet.hardware.gpio_sclk) ||
        !GPIO_IS_VALID_OUTPUT_GPIO(config->pet.hardware.gpio_cs) ||
        !GPIO_IS_VALID_OUTPUT_GPIO(config->pet.hardware.gpio_dc) ||
        !GPIO_IS_VALID_OUTPUT_GPIO(config->pet.hardware.gpio_reset)) {
        return PET_STATUS_INVALID_ARGUMENT;
    }
    memset(backend, 0, sizeof(*backend));
    backend->config = *config;
    display->context = backend;
    display->ops = &display_ops;
    return PET_STATUS_OK;
}

uint64_t pet_esp32_display_last_flush_us(const pet_esp32_display_t *backend)
{
    return backend == NULL ? 0U : backend->last_flush_us;
}

uint32_t pet_esp32_display_actual_frequency_hz(const pet_esp32_display_t *backend)
{
    return backend == NULL ? 0U : backend->actual_spi_frequency_hz;
}

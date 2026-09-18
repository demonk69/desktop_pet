#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hal/backlight.h"
#include "hal/display.h"
#include "pet/pet_app.h"
#include "pet_compiled_assets.h"
#include "pet_esp32_backlight.h"
#include "pet_esp32_board_config.h"
#include "pet_esp32_display.h"
#include "pet_esp32_runtime.h"
#include "ui/pet_renderer.h"

static const char *TAG = "desktop_pet";

static void stop_on_error(const char *step, pet_status_t status)
{
    if (status == PET_STATUS_OK) {
        return;
    }
    ESP_LOGE(TAG, "%s failed: status=%d", step, (int)status);
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(1000U));
    }
}

void app_main(void)
{
    static pet_esp32_board_config_t board;
    static pet_esp32_backlight_t backlight_backend;
    static pet_esp32_display_t display_backend;
    static pet_backlight_t backlight;
    static pet_display_t display;
    static pet_app_t app;
    static pet_compiled_asset_provider_t assets;
    static pet_renderer_t renderer;
    const pet_renderer_theme_t theme = { 0x18C3U, 0xFEC0U, 0x2104U, 0xF9A6U };

    ESP_LOGI(TAG, "Desktop Pet ESP32");
    ESP_LOGI(TAG, "MCU: ESP32-S3");
    ESP_LOGI(TAG, "Display: ST7789 240x240");

    pet_esp32_board_config_init(&board);
    ESP_LOGI(TAG, "LCD: SPI=%u Hz staging=%u bytes DMA=%s block=%u lines",
             (unsigned)board.pet.hardware.spi_frequency_hz,
             (unsigned)board.staging_buffer_bytes,
             board.lcd_dma_enabled ? "on" : "off", (unsigned)board.block_height);
    stop_on_error("Config validation", pet_config_validate(&board.pet));
    stop_on_error("Backlight create",
                  pet_esp32_backlight_create(&backlight_backend, &backlight,
                                             board.pet.hardware.gpio_backlight,
                                             board.backlight_active_high));
    stop_on_error("Backlight init", pet_backlight_init(&backlight));
    stop_on_error("Display create",
                  pet_esp32_display_create(&display_backend, &display, &board));
    stop_on_error("Display init", pet_display_init(&display));
    ESP_LOGI(TAG, "Display init OK; actual SPI=%u Hz",
             (unsigned)pet_esp32_display_actual_frequency_hz(&display_backend));

    stop_on_error("App init", pet_app_init(&app, &board.pet.app));
    stop_on_error("Compiled assets init", pet_compiled_asset_provider_init(&assets));
    stop_on_error("Renderer init", pet_renderer_init(&renderer, &display, &theme));
    pet_renderer_set_asset_provider(&renderer,
                                    pet_compiled_asset_provider_interface(&assets));
    ESP_LOGI(TAG, "Renderer init OK");
    stop_on_error("Runtime", pet_esp32_runtime_run(&app, &renderer, &display_backend,
                                                   &backlight));
}

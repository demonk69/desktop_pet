#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hal/backlight.h"
#include "hal/display.h"
#include "pet/pet_app.h"
#include "pet_compiled_assets.h"
#include "pet_esp32_backlight.h"
#include "pet_esp32_board_config.h"
#include "pet_esp32_diag.h"
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

static pet_status_t log_lcd_transfer_plan(const pet_esp32_board_config_t *board)
{
    const pet_hardware_config_t *hardware;
    size_t framebuffer_bytes;
    size_t transaction_bytes;
    size_t full_transactions;
    size_t final_tail_bytes;
    size_t transaction_count;
    size_t last_transaction_bytes;
    const char *transport = pet_esp32_diag_transport_name();
    if (board == NULL) {
        return PET_STATUS_INVALID_ARGUMENT;
    }
    hardware = &board->pet.hardware;
    framebuffer_bytes = hardware->framebuffer_bytes;
    transaction_bytes = board->block_height > 0U
                            ? (size_t)hardware->width * board->block_height *
                                  sizeof(uint16_t)
                            : board->staging_buffer_bytes;
    if (framebuffer_bytes == 0U || transaction_bytes == 0U) {
        return PET_STATUS_INVALID_ARGUMENT;
    }
    full_transactions = framebuffer_bytes / transaction_bytes;
    final_tail_bytes = framebuffer_bytes % transaction_bytes;
    transaction_count = full_transactions + (final_tail_bytes > 0U ? 1U : 0U);
    last_transaction_bytes = final_tail_bytes > 0U ? final_tail_bytes : transaction_bytes;

    ESP_LOGI(TAG,
             "LCD frame transfer: framebuffer=%u bytes transactions=%u "
             "last_transaction=%u bytes",
             (unsigned)framebuffer_bytes, (unsigned)transaction_count,
             (unsigned)last_transaction_bytes);

    if (strcmp(transport, "V05") == 0) {
        if (final_tail_bytes != 512U || transaction_count != 29U) {
            ESP_LOGE(TAG,
                     "V05 transfer plan mismatch: transactions=%u final_tail=%u bytes",
                     (unsigned)transaction_count, (unsigned)final_tail_bytes);
            return PET_STATUS_INVALID_ARGUMENT;
        }
        ESP_LOGI(TAG,
                 "LCD DIAG TRANSPORT V05: full_transactions=%u final_tail=%u bytes",
                 (unsigned)full_transactions, (unsigned)final_tail_bytes);
    } else if (strcmp(transport, "ROW8") == 0) {
        if (board->block_height != 8U || board->staging_buffer_bytes != transaction_bytes ||
            final_tail_bytes != 0U || transaction_count != 30U) {
            ESP_LOGE(TAG,
                     "ROW8 transfer plan mismatch: block=%u staging=%u "
                     "transactions=%u final_tail=%u bytes",
                     (unsigned)board->block_height, (unsigned)board->staging_buffer_bytes,
                     (unsigned)transaction_count, (unsigned)final_tail_bytes);
            return PET_STATUS_INVALID_ARGUMENT;
        }
        ESP_LOGI(TAG,
                 "LCD DIAG TRANSPORT ROW8: blocks=%u block_bytes=%u "
                 "partial_tail=%u bytes",
                 (unsigned)transaction_count, (unsigned)transaction_bytes,
                 (unsigned)final_tail_bytes);
    }
    return PET_STATUS_OK;
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
    ESP_LOGI(TAG, "LCD DIAGNOSTIC MODE: %s", pet_esp32_diag_mode_name());
    ESP_LOGI(TAG, "LCD DIAGNOSTIC TRANSPORT: %s", pet_esp32_diag_transport_name());
    if (!pet_esp32_diag_mode_is_valid()) {
        stop_on_error("Unknown LCD diagnostic mode", PET_STATUS_INVALID_ARGUMENT);
    }
    if (!pet_esp32_diag_transport_is_valid()) {
        stop_on_error("Unknown LCD diagnostic transport", PET_STATUS_INVALID_ARGUMENT);
    }

    pet_esp32_board_config_init(&board);
    ESP_LOGI(TAG, "LCD: SPI=%u Hz staging=%u bytes DMA=%s block=%u lines",
             (unsigned)board.pet.hardware.spi_frequency_hz,
             (unsigned)board.staging_buffer_bytes,
             board.lcd_dma_enabled ? "on" : "off", (unsigned)board.block_height);
    stop_on_error("LCD transfer plan", log_lcd_transfer_plan(&board));
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

    if (pet_esp32_diag_is_static()) {
        /* Backlight is turned on once and never toggled again. The static
           diagnostic owns the display from here on and never returns. */
        stop_on_error("Backlight on", pet_backlight_set_brightness(&backlight, 100U));
        stop_on_error("Static diagnostic",
                      pet_esp32_diag_run_static(&display, pet_esp32_diag_static_repeat()));
    }

    stop_on_error("App init", pet_app_init(&app, &board.pet.app));
    stop_on_error("Compiled assets init", pet_compiled_asset_provider_init(&assets));
    stop_on_error("Renderer init", pet_renderer_init(&renderer, &display, &theme));
    pet_renderer_set_asset_provider(&renderer,
                                    pet_compiled_asset_provider_interface(&assets));
    ESP_LOGI(TAG, "Renderer init OK");
    stop_on_error("Runtime", pet_esp32_runtime_run(&app, &renderer, &display_backend,
                                                   &backlight));
}

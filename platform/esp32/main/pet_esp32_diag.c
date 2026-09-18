#include "pet_esp32_diag.h"

#include <inttypes.h>
#include <stdint.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define DIAG_TAG          "lcd_diag"
#define DIAG_CORNER_SIZE  40U
#define DIAG_BOTTOM_LINES 32U
#define DIAG_REPORT_MS    5000U

static pet_status_t fill_rect(pet_display_t *display, int x, int y, int width,
                              int height, uint16_t color)
{
    int row;
    int column;
    for (row = 0; row < height; row++) {
        for (column = 0; column < width; column++) {
            pet_status_t status = pet_display_draw_pixel(display, x + column, y + row,
                                                         color);
            if (status != PET_STATUS_OK) {
                return status;
            }
        }
    }
    return PET_STATUS_OK;
}

/* Fixed test frame: black background, four corner color blocks, central cross,
 * bright 32-line band at the bottom and a separate color on the last line. */
static pet_status_t draw_pattern(pet_display_t *display)
{
    int width = pet_display_width(display);
    int height = pet_display_height(display);
    pet_status_t status;
    if (width <= 0 || height <= 0) {
        return PET_STATUS_NOT_INITIALIZED;
    }
    status = pet_display_clear(display, 0x0000U);
    if (status == PET_STATUS_OK) {
        status = fill_rect(display, 0, 0, DIAG_CORNER_SIZE, DIAG_CORNER_SIZE, 0xF800U);
    }
    if (status == PET_STATUS_OK) {
        status = fill_rect(display, width - DIAG_CORNER_SIZE, 0, DIAG_CORNER_SIZE,
                           DIAG_CORNER_SIZE, 0x07E0U);
    }
    if (status == PET_STATUS_OK) {
        status = fill_rect(display, 0, height - DIAG_CORNER_SIZE, DIAG_CORNER_SIZE,
                           DIAG_CORNER_SIZE, 0x001FU);
    }
    if (status == PET_STATUS_OK) {
        status = fill_rect(display, width - DIAG_CORNER_SIZE, height - DIAG_CORNER_SIZE,
                           DIAG_CORNER_SIZE, DIAG_CORNER_SIZE, 0xFFFFU);
    }
    if (status == PET_STATUS_OK) {
        status = fill_rect(display, 0, height / 2 - 2, width, 4, 0xFFE0U);
    }
    if (status == PET_STATUS_OK) {
        status = fill_rect(display, width / 2 - 2, 0, 4, height, 0xFFE0U);
    }
    if (status == PET_STATUS_OK) {
        status = fill_rect(display, 0, height - DIAG_BOTTOM_LINES, width,
                           DIAG_BOTTOM_LINES - 1U, 0xF81FU);
    }
    if (status == PET_STATUS_OK) {
        status = fill_rect(display, 0, height - 1, width, 1, 0x07FFU);
    }
    return status;
}

pet_status_t pet_esp32_diag_run_static(pet_display_t *display, bool repeat_flush)
{
    const char *mode_name = repeat_flush ? "STATIC_REPEAT" : "STATIC_ONCE";
    uint64_t render_count = 0U;
    uint64_t flush_count = 0U;
    uint32_t report_ms = 0U;
    pet_status_t status = draw_pattern(display);
    if (status != PET_STATUS_OK) {
        return status;
    }
    render_count++;
    status = pet_display_flush(display);
    if (status != PET_STATUS_OK) {
        return status;
    }
    flush_count++;
    ESP_LOGI(DIAG_TAG, "%s: render_count=%" PRIu64 " flush_count=%" PRIu64, mode_name,
             render_count, flush_count);

    for (;;) {
        const TickType_t delay_ms = repeat_flush ? 100U : 1000U;
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
        if (repeat_flush) {
            status = pet_display_flush(display);
            if (status != PET_STATUS_OK) {
                return status;
            }
            flush_count++;
        }
        report_ms += delay_ms;
        if (report_ms >= DIAG_REPORT_MS) {
            report_ms = 0U;
            ESP_LOGI(DIAG_TAG, "%s alive: render_count=%" PRIu64 " flush_count=%" PRIu64,
                     mode_name, render_count, flush_count);
        }
    }
}

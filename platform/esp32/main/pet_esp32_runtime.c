#include "pet_esp32_runtime.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "pet_esp32_time.h"

#if !defined(PET_ESP32_TARGET_FRAME_US) || PET_ESP32_TARGET_FRAME_US <= 0
#error "PET_ESP32_TARGET_FRAME_US must be a positive integer"
#endif

#define TARGET_FRAME_US   ((uint64_t)PET_ESP32_TARGET_FRAME_US)
#define REPORT_INTERVAL_US 1000000ULL

typedef struct {
    uint64_t frames;
    uint64_t render_count;
    uint64_t flush_count;
    uint64_t app_total_us;
    uint64_t app_max_us;
    uint64_t render_total_us;
    uint64_t render_max_us;
    uint64_t flush_total_us;
    uint64_t flush_max_us;
    uint64_t frame_total_us;
    uint64_t frame_max_us;
} runtime_stats_t;

static const char *TAG = "pet_runtime";

static uint64_t maximum(uint64_t left, uint64_t right)
{
    return left > right ? left : right;
}

static uint64_t average(uint64_t total, uint64_t count)
{
    return count == 0ULL ? 0ULL : total / count;
}

static void report_stats(const runtime_stats_t *stats, uint64_t elapsed_us)
{
    uint64_t render_avg_us;
    uint64_t flush_avg_us;
    uint64_t frame_avg_us;
    uint64_t fps_tenths = stats->frames * 10000000ULL / elapsed_us;

    if (stats->frames == 0ULL) {
        return;
    }
    render_avg_us = average(stats->render_total_us, stats->render_count);
    flush_avg_us = average(stats->flush_total_us, stats->flush_count);
    frame_avg_us = average(stats->frame_total_us, stats->frames);

    ESP_LOGI(TAG,
             "PERF1 render_count=%" PRIu64 " flush_count=%" PRIu64
             " fps=%" PRIu64 ".%" PRIu64
             " render_ms(avg/max)=%" PRIu64 ".%03" PRIu64 "/%" PRIu64 ".%03" PRIu64
             " flush_ms(avg/max)=%" PRIu64 ".%03" PRIu64 "/%" PRIu64 ".%03" PRIu64
             " frame_ms(avg/max)=%" PRIu64 ".%03" PRIu64 "/%" PRIu64 ".%03" PRIu64
             " heap_current=%u psram_free=%u",
             stats->render_count, stats->flush_count,
             fps_tenths / 10ULL, fps_tenths % 10ULL,
             render_avg_us / 1000ULL, render_avg_us % 1000ULL,
             stats->render_max_us / 1000ULL, stats->render_max_us % 1000ULL,
             flush_avg_us / 1000ULL, flush_avg_us % 1000ULL,
             stats->flush_max_us / 1000ULL, stats->flush_max_us % 1000ULL,
             frame_avg_us / 1000ULL, frame_avg_us % 1000ULL,
             stats->frame_max_us / 1000ULL, stats->frame_max_us % 1000ULL,
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
}

pet_status_t pet_esp32_runtime_run(pet_app_t *app, pet_renderer_t *renderer,
                                    pet_esp32_display_t *display_backend,
                                    pet_backlight_t *backlight,
                                    pet_esp32_rotary_t *rotary,
                                    const pet_time_service_t *time_service)
{
    uint64_t run_start_us;
    uint64_t previous_frame_us;
    uint64_t report_start_us;
    uint64_t last_time_update_us;
    pet_time_snapshot_t time_snapshot = { 0 };
    runtime_stats_t stats = { 0 };
    pet_app_snapshot_t initial_snapshot;
    pet_state_t previous_state;
    pet_animation_id_t previous_animation;
    pet_ui_scene_id_t previous_visible_scene;
    bool backlight_enabled = false;

    if (app == NULL || renderer == NULL || display_backend == NULL ||
        backlight == NULL || time_service == NULL) {
        return PET_STATUS_INVALID_ARGUMENT;
    }
    run_start_us = pet_esp32_time_now_us();
    previous_frame_us = run_start_us;
    report_start_us = run_start_us;
    last_time_update_us = run_start_us;
    (void)pet_time_service_get_snapshot(time_service, &time_snapshot);
    pet_app_set_time_snapshot(app, &time_snapshot);
    initial_snapshot = pet_app_snapshot(app);
    previous_state = initial_snapshot.state;
    previous_animation = initial_snapshot.animation;
    previous_visible_scene = pet_ui_snapshot_visible_scene(&initial_snapshot.ui);
    {
        uint64_t target_fps_tenths = 10000000ULL / TARGET_FRAME_US;
        ESP_LOGI(TAG,
                 "Runtime started: target=%" PRIu64 ".%" PRIu64
                 " FPS, synchronous full-frame flush",
                 target_fps_tenths / 10ULL, target_fps_tenths % 10ULL);
    }

    for (;;) {
        uint64_t frame_start_us = pet_esp32_time_now_us();
        uint64_t delta_us = frame_start_us - previous_frame_us;
        uint64_t app_start_us;
        uint64_t app_us;
        uint64_t render_start_us;
        uint64_t render_call_us;
        uint64_t flush_us;
        uint64_t render_us;
        uint64_t frame_us;
        uint64_t now_us;
        uint32_t delta_ms = delta_us > 1000000ULL ? 1000U : (uint32_t)(delta_us / 1000ULL);
        pet_app_snapshot_t snapshot;
        pet_status_t status;

        previous_frame_us = frame_start_us;

        app_start_us = pet_esp32_time_now_us();
        pet_esp32_rotary_drain_events(rotary, app);
        pet_app_update(app, delta_ms);
        app_us = pet_esp32_time_now_us() - app_start_us;
        if (frame_start_us - last_time_update_us >= 1000000ULL) {
            last_time_update_us = frame_start_us;
            (void)pet_time_service_get_snapshot(time_service, &time_snapshot);
            pet_app_set_time_snapshot(app, &time_snapshot);
        }
        snapshot = pet_app_snapshot(app);
        {
            pet_ui_scene_id_t visible_scene =
                pet_ui_snapshot_visible_scene(&snapshot.ui);
            if (visible_scene != previous_visible_scene) {
                ESP_LOGI(TAG, "SCENE: %s -> %s",
                         pet_ui_scene_name(previous_visible_scene),
                         pet_ui_scene_name(visible_scene));
                previous_visible_scene = visible_scene;
            }
        }
        if (snapshot.state != previous_state || snapshot.animation != previous_animation) {
            if (snapshot.state == PET_STATE_SLEEP && previous_state != PET_STATE_SLEEP) {
                status = pet_backlight_sleep(backlight);
                if (status != PET_STATUS_OK) {
                    return status;
                }
            } else if (previous_state == PET_STATE_SLEEP && snapshot.state != PET_STATE_SLEEP) {
                status = pet_backlight_wake(backlight);
                if (status != PET_STATUS_OK) {
                    return status;
                }
            }
            ESP_LOGI(TAG, "State=%s animation=%s", pet_state_name(snapshot.state),
                     pet_animation_name(snapshot.animation));
            previous_state = snapshot.state;
            previous_animation = snapshot.animation;
        }

        render_start_us = pet_esp32_time_now_us();
        status = pet_renderer_render(renderer, &snapshot);
        render_call_us = pet_esp32_time_now_us() - render_start_us;
        if (status != PET_STATUS_OK) {
            return status;
        }
        flush_us = pet_esp32_display_last_flush_us(display_backend);
        render_us = render_call_us >= flush_us ? render_call_us - flush_us : 0U;
        if (!backlight_enabled) {
            status = pet_backlight_wake(backlight);
            if (status != PET_STATUS_OK) {
                return status;
            }
            backlight_enabled = true;
            ESP_LOGI(TAG, "First frame displayed; backlight enabled");
        }

        now_us = pet_esp32_time_now_us();
        frame_us = now_us - frame_start_us;
        stats.frames++;
        stats.render_count++;
        stats.flush_count++;
        stats.app_total_us += app_us;
        stats.app_max_us = maximum(stats.app_max_us, app_us);
        stats.render_total_us += render_us;
        stats.render_max_us = maximum(stats.render_max_us, render_us);
        stats.flush_total_us += flush_us;
        stats.flush_max_us = maximum(stats.flush_max_us, flush_us);
        stats.frame_total_us += frame_us;
        stats.frame_max_us = maximum(stats.frame_max_us, frame_us);

        if (now_us - report_start_us >= REPORT_INTERVAL_US) {
            report_stats(&stats, now_us - report_start_us);
            stats = (runtime_stats_t){ 0 };
            report_start_us = now_us;
        }
        if (frame_us < TARGET_FRAME_US) {
            uint64_t remaining_ms = (TARGET_FRAME_US - frame_us + 999ULL) / 1000ULL;
            vTaskDelay(pdMS_TO_TICKS((uint32_t)remaining_ms) + 1U);
        } else {
            vTaskDelay(1U);
        }
    }
}

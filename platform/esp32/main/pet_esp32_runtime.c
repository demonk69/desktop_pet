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
#define REPORT_INTERVAL_US 5000000ULL
#define DEMO_CYCLE_MS 22000ULL

typedef struct {
    uint64_t frames;
    uint64_t app_total_us;
    uint64_t app_max_us;
    uint64_t render_total_us;
    uint64_t render_max_us;
    uint64_t flush_total_us;
    uint64_t flush_max_us;
    uint64_t frame_total_us;
    uint64_t frame_max_us;
} runtime_stats_t;

typedef struct {
    uint64_t cycle;
    bool happy_posted;
    bool sleep_posted;
    bool wake_posted;
} demo_state_t;

static const char *TAG = "pet_runtime";

static uint64_t maximum(uint64_t left, uint64_t right)
{
    return left > right ? left : right;
}

static void post_demo_event(pet_app_t *app, pet_event_type_t type, const char *name)
{
    const pet_event_t event = { .type = type, .timestamp_ms = app->now_ms };
    if (pet_app_post_event(app, &event)) {
        ESP_LOGI(TAG, "Demo event: %s", name);
    } else {
        ESP_LOGW(TAG, "Demo event queue full: %s", name);
    }
}

static void update_demo(pet_app_t *app, demo_state_t *demo, uint64_t elapsed_ms)
{
    uint64_t cycle = elapsed_ms / DEMO_CYCLE_MS;
    uint64_t cycle_ms = elapsed_ms % DEMO_CYCLE_MS;
    if (cycle != demo->cycle) {
        demo->cycle = cycle;
        demo->happy_posted = false;
        demo->sleep_posted = false;
        demo->wake_posted = false;
    }
    if (!demo->happy_posted && cycle_ms >= 10000ULL) {
        post_demo_event(app, PET_EVENT_HAPPY, "HAPPY");
        demo->happy_posted = true;
    }
    if (!demo->sleep_posted && cycle_ms >= 14000ULL) {
        post_demo_event(app, PET_EVENT_SLEEP, "SLEEP");
        demo->sleep_posted = true;
    }
    if (!demo->wake_posted && cycle_ms >= 17000ULL) {
        post_demo_event(app, PET_EVENT_WAKE, "WAKE");
        demo->wake_posted = true;
    }
}

static void report_stats(const runtime_stats_t *stats, uint64_t elapsed_us)
{
    uint64_t fps_tenths = stats->frames * 10000000ULL / elapsed_us;
    ESP_LOGI(TAG,
             "PERF fps=%" PRIu64 ".%" PRIu64
             " app_us(avg/max)=%" PRIu64 "/%" PRIu64
             " render_us(avg/max)=%" PRIu64 "/%" PRIu64
             " flush_us(avg/max)=%" PRIu64 "/%" PRIu64
             " frame_us(avg/max)=%" PRIu64 "/%" PRIu64
             " heap_internal=%u heap_internal_min=%u psram=%u",
             fps_tenths / 10ULL, fps_tenths % 10ULL,
             stats->app_total_us / stats->frames, stats->app_max_us,
             stats->render_total_us / stats->frames, stats->render_max_us,
             stats->flush_total_us / stats->frames, stats->flush_max_us,
             stats->frame_total_us / stats->frames, stats->frame_max_us,
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
             (unsigned)heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL |
                                                       MALLOC_CAP_8BIT),
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
}

pet_status_t pet_esp32_runtime_run(pet_app_t *app, pet_renderer_t *renderer,
                                   pet_esp32_display_t *display_backend,
                                   pet_backlight_t *backlight)
{
    uint64_t run_start_us;
    uint64_t previous_frame_us;
    uint64_t report_start_us;
    runtime_stats_t stats = { 0 };
    demo_state_t demo = { 0 };
    pet_state_t previous_state;
    pet_animation_id_t previous_animation;
    bool backlight_enabled = false;

    if (app == NULL || renderer == NULL || display_backend == NULL || backlight == NULL) {
        return PET_STATUS_INVALID_ARGUMENT;
    }
    run_start_us = pet_esp32_time_now_us();
    previous_frame_us = run_start_us;
    report_start_us = run_start_us;
    previous_state = pet_app_snapshot(app).state;
    previous_animation = pet_app_snapshot(app).animation;
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
        update_demo(app, &demo, (frame_start_us - run_start_us) / 1000ULL);

        app_start_us = pet_esp32_time_now_us();
        pet_app_update(app, delta_ms);
        app_us = pet_esp32_time_now_us() - app_start_us;
        snapshot = pet_app_snapshot(app);
        if (snapshot.state != previous_state || snapshot.animation != previous_animation) {
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
            status = pet_backlight_set_brightness(backlight, 100U);
            if (status != PET_STATUS_OK) {
                return status;
            }
            backlight_enabled = true;
            ESP_LOGI(TAG, "First frame displayed; backlight enabled");
        }

        now_us = pet_esp32_time_now_us();
        frame_us = now_us - frame_start_us;
        stats.frames++;
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

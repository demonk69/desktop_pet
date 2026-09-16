#include <stdio.h>

#include "config/pet_config.h"
#include "framebuffer_display.h"
#include "pet/pet_app.h"
#include "ui/pet_renderer.h"

static void post_simple_event(pet_app_t *app, pet_event_type_t type)
{
    pet_event_t event = { .type = type, .timestamp_ms = app->now_ms, .data.code = 0 };
    (void)pet_app_post_event(app, &event);
}

int main(int argc, char **argv)
{
    const char *output = argc > 1 ? argv[1] : "pet_simulator.ppm";
    pet_config_t config;
    pet_app_t app;
    pet_display_t display;
    framebuffer_display_t framebuffer;
    pet_renderer_t renderer;
    pet_renderer_theme_t theme = { 0x18C3U, 0xFEC0U, 0x2104U, 0xF9A6U };
    pet_state_t previous = PET_STATE_COUNT;
    int tick;

    pet_config_set_development_defaults(&config);
    if (pet_config_validate(&config) != PET_STATUS_OK ||
        pet_app_init(&app, &config.app) != PET_STATUS_OK ||
        framebuffer_display_create(&framebuffer, &display, config.hardware.width,
                                   config.hardware.height) != PET_STATUS_OK ||
        pet_display_init(&display) != PET_STATUS_OK ||
        pet_renderer_init(&renderer, &display, &theme) != PET_STATUS_OK) {
        (void)fprintf(stderr, "simulator initialization failed\n");
        return 1;
    }

    for (tick = 0; tick < 80; tick++) {
        pet_app_snapshot_t snapshot;
        if (tick == 28) {
            post_simple_event(&app, PET_EVENT_BUTTON);
        } else if (tick == 48) {
            post_simple_event(&app, PET_EVENT_SLEEP);
        } else if (tick == 65) {
            post_simple_event(&app, PET_EVENT_WAKE);
        }
        pet_app_update(&app, 100U);
        snapshot = pet_app_snapshot(&app);
        if (snapshot.state != previous) {
            (void)printf("%5u ms  state=%s  animation=%d\n", app.now_ms,
                         pet_state_name(snapshot.state), (int)snapshot.animation);
            previous = snapshot.state;
        }
        (void)pet_renderer_render(&renderer, &snapshot);
    }

    if (framebuffer_display_write_ppm(&framebuffer, output) != PET_STATUS_OK) {
        (void)fprintf(stderr, "failed to write %s\n", output);
        framebuffer_display_destroy(&framebuffer);
        return 1;
    }
    (void)printf("wrote final frame to %s\n", output);
    framebuffer_display_destroy(&framebuffer);
    return 0;
}

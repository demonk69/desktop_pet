#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL.h>

#include "config/pet_config.h"
#include "file_asset_provider.h"
#include "pet/pet_app.h"
#include "sdl_display.h"
#include "services/pet_time_service.h"
#include "simulator_input.h"
#include "ui/pet_renderer.h"

#ifndef PET_DEFAULT_ASSET_ROOT
#define PET_DEFAULT_ASSET_ROOT "assets/pet"
#endif

typedef struct {
    int scale;
    int max_frames;
    const char *asset_root;
} simulator_options_t;

static void print_help(void)
{
    (void)printf(
        "Desktop Pet Simulator\n\n"
        "SPACE  interact\n"
        "H      happy\n"
        "S      sleep\n"
        "W      wake\n"
        "M      message (Hello Pet)\n"
        "LEFT   look left\n"
        "RIGHT  look right\n"
        "ESC    quit\n\n");
}

static bool parse_options(int argc, char **argv, simulator_options_t *options)
{
    int index;
    *options = (simulator_options_t){ 3, 0, PET_DEFAULT_ASSET_ROOT };
    for (index = 1; index < argc; index++) {
        if (strcmp(argv[index], "--scale") == 0 && index + 1 < argc) {
            options->scale = atoi(argv[++index]);
        } else if (strcmp(argv[index], "--assets") == 0 && index + 1 < argc) {
            options->asset_root = argv[++index];
        } else if (strcmp(argv[index], "--frames") == 0 && index + 1 < argc) {
            options->max_frames = atoi(argv[++index]);
        } else if (strcmp(argv[index], "--help") == 0) {
            print_help();
            return false;
        } else {
            (void)fprintf(stderr, "unknown or incomplete option: %s\n", argv[index]);
            return false;
        }
    }
    return options->scale >= 1 && options->scale <= 8 && options->max_frames >= 0;
}

int main(int argc, char **argv)
{
    const uint32_t target_frame_ms = 1000U / 60U;
    simulator_options_t options;
    pet_config_t config;
    pet_app_t app;
    pet_display_t display;
    sdl_display_t sdl_backend = { 0 };
    pet_file_asset_provider_t file_assets = { 0 };
    pet_renderer_t renderer;
    pet_renderer_theme_t theme = { 0x18C3U, 0xFEC0U, 0x2104U, 0xF9A6U };
    pet_time_service_t time_service;
    pet_time_snapshot_t time_snapshot = { 0 };
    uint32_t last_time_update_ms = 0;
    pet_state_t previous_state = PET_STATE_COUNT;
    pet_animation_id_t previous_animation = PET_ANIM_COUNT;
    uint64_t previous_time;
    bool running = true;
    int frame_count = 0;
    int result = 1;

    if (!parse_options(argc, argv, &options)) {
        return argc > 1 && strcmp(argv[1], "--help") == 0 ? 0 : 2;
    }
    pet_config_set_development_defaults(&config);
    if (pet_config_validate(&config) != PET_STATUS_OK ||
        pet_app_init(&app, &config.app) != PET_STATUS_OK ||
        pet_file_asset_provider_init(&file_assets, options.asset_root, "manifest.txt") !=
            PET_STATUS_OK ||
        sdl_display_create(&sdl_backend, &display, config.hardware.width,
                           config.hardware.height, options.scale,
                           "Desktop Pet Simulator") != PET_STATUS_OK ||
        pet_display_init(&display) != PET_STATUS_OK ||
        pet_renderer_init(&renderer, &display, &theme) != PET_STATUS_OK) {
        (void)fprintf(stderr, "simulator initialization failed: %s\n", SDL_GetError());
        pet_file_asset_provider_destroy(&file_assets);
        sdl_display_destroy(&sdl_backend);
        return 1;
    }
    pet_renderer_set_asset_provider(&renderer,
                                    pet_file_asset_provider_interface(&file_assets));
    if (pet_time_service_init_system(&time_service) != PET_STATUS_OK) {
        (void)fprintf(stderr, "time service initialization failed\n");
        pet_file_asset_provider_destroy(&file_assets);
        sdl_display_destroy(&sdl_backend);
        return 1;
    }
    print_help();
    previous_time = SDL_GetTicks64();

    while (running) {
        uint64_t frame_start = SDL_GetTicks64();
        uint64_t current_time = frame_start;
        uint64_t raw_delta = current_time - previous_time;
        uint32_t delta_ms = raw_delta > 100U ? 100U : (uint32_t)raw_delta;
        SDL_Event platform_event;
        pet_app_snapshot_t snapshot;

        previous_time = current_time;
        while (SDL_PollEvent(&platform_event) != 0) {
            pet_event_t pet_event;
            bool quit_requested;
            if (simulator_input_translate(&platform_event, app.now_ms, &pet_event,
                                          &quit_requested)) {
                (void)printf("[EVENT] %s\n", simulator_event_name(&pet_event));
                if (!pet_app_post_event(&app, &pet_event)) {
                    (void)fprintf(stderr, "[EVENT] queue full, event dropped\n");
                }
            }
            if (quit_requested) {
                running = false;
            }
        }

        pet_app_update(&app, delta_ms);
        if (app.now_ms - last_time_update_ms >= 1000U) {
            last_time_update_ms = app.now_ms;
            (void)pet_time_service_get_snapshot(&time_service, &time_snapshot);
            pet_app_set_time_snapshot(&app, &time_snapshot);
        }
        snapshot = pet_app_snapshot(&app);
        if (snapshot.state != previous_state) {
            (void)printf("[PET] state: %s -> %s\n", pet_state_name(previous_state),
                         pet_state_name(snapshot.state));
            previous_state = snapshot.state;
        }
        if (snapshot.animation != previous_animation) {
            (void)printf("[ANIM] play: %s\n", pet_animation_name(snapshot.animation));
            previous_animation = snapshot.animation;
        }
        if (pet_renderer_render(&renderer, &snapshot) != PET_STATUS_OK) {
            (void)fprintf(stderr, "render failed: %s\n", SDL_GetError());
            result = 1;
            break;
        }

        frame_count++;
        if (options.max_frames > 0 && frame_count >= options.max_frames) {
            running = false;
        }
        current_time = SDL_GetTicks64();
        if (current_time - frame_start < target_frame_ms) {
            SDL_Delay(target_frame_ms - (uint32_t)(current_time - frame_start));
        }
    }
    if (!running) {
        result = 0;
    }
    pet_file_asset_provider_destroy(&file_assets);
    sdl_display_destroy(&sdl_backend);
    return result;
}

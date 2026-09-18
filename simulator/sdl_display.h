#ifndef PET_SDL_DISPLAY_H
#define PET_SDL_DISPLAY_H

#include <stdbool.h>
#include <stdint.h>

#include <SDL.h>

#include "hal/display.h"

typedef struct {
    int width;
    int height;
    int window_scale;
    const char *title;
    uint8_t rotation;
    uint8_t brightness;
    uint16_t *pixels;
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    bool owns_video_subsystem;
} sdl_display_t;

pet_status_t sdl_display_create(sdl_display_t *backend, pet_display_t *display,
                                int width, int height, int window_scale,
                                const char *title);
void sdl_display_destroy(sdl_display_t *backend);

#endif

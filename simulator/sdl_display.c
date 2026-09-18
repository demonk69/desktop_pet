#include "sdl_display.h"

#include <stdlib.h>

static pet_status_t sdl_backend_init(void *context)
{
    sdl_display_t *backend = context;
    size_t pixel_count;

    if (backend == NULL || backend->width <= 0 || backend->height <= 0 ||
        backend->window_scale <= 0) {
        return PET_STATUS_INVALID_ARGUMENT;
    }
    if ((SDL_WasInit(SDL_INIT_VIDEO) & SDL_INIT_VIDEO) == 0U) {
        if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0) {
            return PET_STATUS_IO_ERROR;
        }
        backend->owns_video_subsystem = true;
    }
    (void)SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
    backend->window = SDL_CreateWindow(
        backend->title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        backend->width * backend->window_scale, backend->height * backend->window_scale,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (backend->window == NULL) {
        return PET_STATUS_IO_ERROR;
    }
    backend->renderer = SDL_CreateRenderer(
        backend->window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (backend->renderer == NULL) {
        backend->renderer = SDL_CreateRenderer(backend->window, -1, SDL_RENDERER_SOFTWARE);
    }
    if (backend->renderer == NULL ||
        SDL_RenderSetLogicalSize(backend->renderer, backend->width, backend->height) != 0) {
        return PET_STATUS_IO_ERROR;
    }
    backend->texture = SDL_CreateTexture(backend->renderer, SDL_PIXELFORMAT_RGB565,
                                         SDL_TEXTUREACCESS_STREAMING, backend->width,
                                         backend->height);
    if (backend->texture == NULL) {
        return PET_STATUS_IO_ERROR;
    }
    pixel_count = (size_t)backend->width * (size_t)backend->height;
    backend->pixels = calloc(pixel_count, sizeof(*backend->pixels));
    return backend->pixels == NULL ? PET_STATUS_NO_MEMORY : PET_STATUS_OK;
}

static pet_status_t sdl_clear(void *context, uint16_t color)
{
    sdl_display_t *backend = context;
    size_t index;
    size_t count;
    if (backend == NULL || backend->pixels == NULL) {
        return PET_STATUS_NOT_INITIALIZED;
    }
    count = (size_t)backend->width * (size_t)backend->height;
    for (index = 0U; index < count; index++) {
        backend->pixels[index] = color;
    }
    return PET_STATUS_OK;
}

static pet_status_t sdl_pixel(void *context, int x, int y, uint16_t color)
{
    sdl_display_t *backend = context;
    if (backend == NULL || backend->pixels == NULL) {
        return PET_STATUS_NOT_INITIALIZED;
    }
    if (x >= 0 && x < backend->width && y >= 0 && y < backend->height) {
        backend->pixels[(size_t)y * (size_t)backend->width + (size_t)x] = color;
    }
    return PET_STATUS_OK;
}

static pet_status_t sdl_region(void *context, int x, int y, int width, int height,
                               const uint16_t *pixels, size_t stride)
{
    int row;
    int column;
    for (row = 0; row < height; row++) {
        for (column = 0; column < width; column++) {
            pet_status_t status = sdl_pixel(
                context, x + column, y + row,
                pixels[(size_t)row * stride + (size_t)column]);
            if (status != PET_STATUS_OK) {
                return status;
            }
        }
    }
    return PET_STATUS_OK;
}

static pet_status_t sdl_bitmap(void *context, int x, int y, int width, int height,
                               const uint16_t *pixels)
{
    return sdl_region(context, x, y, width, height, pixels, (size_t)width);
}

static pet_status_t sdl_flush(void *context)
{
    sdl_display_t *backend = context;
    if (backend == NULL || backend->pixels == NULL || backend->texture == NULL ||
        backend->renderer == NULL) {
        return PET_STATUS_NOT_INITIALIZED;
    }
    if (SDL_UpdateTexture(backend->texture, NULL, backend->pixels,
                          backend->width * (int)sizeof(*backend->pixels)) != 0 ||
        SDL_RenderClear(backend->renderer) != 0 ||
        SDL_RenderCopy(backend->renderer, backend->texture, NULL, NULL) != 0) {
        return PET_STATUS_IO_ERROR;
    }
    SDL_RenderPresent(backend->renderer);
    return PET_STATUS_OK;
}

static pet_status_t sdl_rotation(void *context, uint8_t rotation)
{
    sdl_display_t *backend = context;
    if (backend == NULL || rotation > 3U) {
        return PET_STATUS_INVALID_ARGUMENT;
    }
    backend->rotation = rotation;
    return PET_STATUS_OK;
}

static int sdl_width(void *context)
{
    sdl_display_t *backend = context;
    return backend == NULL ? 0 : backend->width;
}

static int sdl_height(void *context)
{
    sdl_display_t *backend = context;
    return backend == NULL ? 0 : backend->height;
}

static pet_status_t sdl_brightness(void *context, uint8_t percent)
{
    sdl_display_t *backend = context;
    uint8_t modulation;
    if (backend == NULL || backend->texture == NULL || percent > 100U) {
        return PET_STATUS_INVALID_ARGUMENT;
    }
    modulation = (uint8_t)((unsigned int)percent * 255U / 100U);
    if (SDL_SetTextureColorMod(backend->texture, modulation, modulation, modulation) != 0) {
        return PET_STATUS_IO_ERROR;
    }
    backend->brightness = percent;
    return PET_STATUS_OK;
}

static const pet_display_ops_t sdl_ops = {
    sdl_backend_init, sdl_clear, sdl_pixel, sdl_bitmap, sdl_region, sdl_flush,
    sdl_rotation, sdl_width, sdl_height, sdl_brightness
};

pet_status_t sdl_display_create(sdl_display_t *backend, pet_display_t *display,
                                int width, int height, int window_scale,
                                const char *title)
{
    if (backend == NULL || display == NULL || title == NULL || width <= 0 || height <= 0 ||
        window_scale <= 0) {
        return PET_STATUS_INVALID_ARGUMENT;
    }
    *backend = (sdl_display_t){ .width = width,
                                .height = height,
                                .window_scale = window_scale,
                                .title = title,
                                .brightness = 100U };
    display->context = backend;
    display->ops = &sdl_ops;
    return PET_STATUS_OK;
}

void sdl_display_destroy(sdl_display_t *backend)
{
    if (backend == NULL) {
        return;
    }
    free(backend->pixels);
    backend->pixels = NULL;
    SDL_DestroyTexture(backend->texture);
    SDL_DestroyRenderer(backend->renderer);
    SDL_DestroyWindow(backend->window);
    backend->texture = NULL;
    backend->renderer = NULL;
    backend->window = NULL;
    if (backend->owns_video_subsystem) {
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
        backend->owns_video_subsystem = false;
    }
}

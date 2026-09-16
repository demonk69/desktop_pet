#ifndef PET_DISPLAY_H
#define PET_DISPLAY_H

#include <stddef.h>
#include <stdint.h>

#include "pet/status.h"

typedef struct pet_display pet_display_t;

typedef struct {
    pet_status_t (*init)(void *context);
    pet_status_t (*clear)(void *context, uint16_t color);
    pet_status_t (*draw_pixel)(void *context, int x, int y, uint16_t color);
    pet_status_t (*draw_bitmap)(void *context, int x, int y, int width, int height,
                                const uint16_t *pixels);
    pet_status_t (*draw_region)(void *context, int x, int y, int width, int height,
                                const uint16_t *pixels, size_t stride_pixels);
    pet_status_t (*flush)(void *context);
    pet_status_t (*set_rotation)(void *context, uint8_t rotation);
    int (*width)(void *context);
    int (*height)(void *context);
    pet_status_t (*set_brightness)(void *context, uint8_t percent);
} pet_display_ops_t;

struct pet_display {
    void *context;
    const pet_display_ops_t *ops;
};

pet_status_t pet_display_init(pet_display_t *display);
pet_status_t pet_display_clear(pet_display_t *display, uint16_t color);
pet_status_t pet_display_draw_pixel(pet_display_t *display, int x, int y, uint16_t color);
pet_status_t pet_display_draw_bitmap(pet_display_t *display, int x, int y, int width,
                                     int height, const uint16_t *pixels);
pet_status_t pet_display_draw_region(pet_display_t *display, int x, int y, int width,
                                     int height, const uint16_t *pixels,
                                     size_t stride_pixels);
pet_status_t pet_display_flush(pet_display_t *display);
pet_status_t pet_display_set_rotation(pet_display_t *display, uint8_t rotation);
int pet_display_width(const pet_display_t *display);
int pet_display_height(const pet_display_t *display);
pet_status_t pet_display_set_brightness(pet_display_t *display, uint8_t percent);

#endif

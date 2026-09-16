#ifndef PET_FRAMEBUFFER_DISPLAY_H
#define PET_FRAMEBUFFER_DISPLAY_H

#include <stdint.h>

#include "hal/display.h"

typedef struct {
    int width;
    int height;
    uint8_t rotation;
    uint8_t brightness;
    uint16_t *pixels;
} framebuffer_display_t;

pet_status_t framebuffer_display_create(framebuffer_display_t *framebuffer,
                                        pet_display_t *display, int width, int height);
void framebuffer_display_destroy(framebuffer_display_t *framebuffer);
pet_status_t framebuffer_display_write_ppm(const framebuffer_display_t *framebuffer,
                                           const char *path);

#endif

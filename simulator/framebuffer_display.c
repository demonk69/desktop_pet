#include "framebuffer_display.h"

#include <stdio.h>
#include <stdlib.h>

static pet_status_t fb_init(void *context)
{
    framebuffer_display_t *fb = context;
    size_t count;
    if (fb == NULL || fb->width <= 0 || fb->height <= 0) {
        return PET_STATUS_INVALID_ARGUMENT;
    }
    count = (size_t)fb->width * (size_t)fb->height;
    fb->pixels = calloc(count, sizeof(*fb->pixels));
    return fb->pixels == NULL ? PET_STATUS_NO_MEMORY : PET_STATUS_OK;
}

static pet_status_t fb_clear(void *context, uint16_t color)
{
    framebuffer_display_t *fb = context;
    size_t index;
    size_t count;
    if (fb == NULL || fb->pixels == NULL) {
        return PET_STATUS_NOT_INITIALIZED;
    }
    count = (size_t)fb->width * (size_t)fb->height;
    for (index = 0U; index < count; index++) {
        fb->pixels[index] = color;
    }
    return PET_STATUS_OK;
}

static pet_status_t fb_pixel(void *context, int x, int y, uint16_t color)
{
    framebuffer_display_t *fb = context;
    if (fb == NULL || fb->pixels == NULL) {
        return PET_STATUS_NOT_INITIALIZED;
    }
    if (x >= 0 && x < fb->width && y >= 0 && y < fb->height) {
        fb->pixels[(size_t)y * (size_t)fb->width + (size_t)x] = color;
    }
    return PET_STATUS_OK;
}

static pet_status_t fb_region(void *context, int x, int y, int width, int height,
                              const uint16_t *pixels, size_t stride)
{
    int row;
    int column;
    for (row = 0; row < height; row++) {
        for (column = 0; column < width; column++) {
            pet_status_t status = fb_pixel(context, x + column, y + row,
                                           pixels[(size_t)row * stride + (size_t)column]);
            if (status != PET_STATUS_OK) {
                return status;
            }
        }
    }
    return PET_STATUS_OK;
}

static pet_status_t fb_bitmap(void *context, int x, int y, int width, int height,
                              const uint16_t *pixels)
{
    return fb_region(context, x, y, width, height, pixels, (size_t)width);
}

static pet_status_t fb_flush(void *context)
{
    framebuffer_display_t *fb = context;
    return fb != NULL && fb->pixels != NULL ? PET_STATUS_OK : PET_STATUS_NOT_INITIALIZED;
}

static pet_status_t fb_rotation(void *context, uint8_t rotation)
{
    framebuffer_display_t *fb = context;
    if (fb == NULL || rotation > 3U) {
        return PET_STATUS_INVALID_ARGUMENT;
    }
    fb->rotation = rotation;
    return PET_STATUS_OK;
}

static int fb_width(void *context)
{
    framebuffer_display_t *fb = context;
    return fb == NULL ? 0 : fb->width;
}

static int fb_height(void *context)
{
    framebuffer_display_t *fb = context;
    return fb == NULL ? 0 : fb->height;
}

static pet_status_t fb_brightness(void *context, uint8_t percent)
{
    framebuffer_display_t *fb = context;
    if (fb == NULL || percent > 100U) {
        return PET_STATUS_INVALID_ARGUMENT;
    }
    fb->brightness = percent;
    return PET_STATUS_OK;
}

static const pet_display_ops_t framebuffer_ops = {
    fb_init, fb_clear, fb_pixel, fb_bitmap, fb_region, fb_flush,
    fb_rotation, fb_width, fb_height, fb_brightness
};

pet_status_t framebuffer_display_create(framebuffer_display_t *framebuffer,
                                        pet_display_t *display, int width, int height)
{
    if (framebuffer == NULL || display == NULL || width <= 0 || height <= 0) {
        return PET_STATUS_INVALID_ARGUMENT;
    }
    framebuffer->width = width;
    framebuffer->height = height;
    framebuffer->rotation = 0U;
    framebuffer->brightness = 100U;
    framebuffer->pixels = NULL;
    display->context = framebuffer;
    display->ops = &framebuffer_ops;
    return PET_STATUS_OK;
}

void framebuffer_display_destroy(framebuffer_display_t *framebuffer)
{
    if (framebuffer != NULL) {
        free(framebuffer->pixels);
        framebuffer->pixels = NULL;
    }
}

pet_status_t framebuffer_display_write_ppm(const framebuffer_display_t *framebuffer,
                                           const char *path)
{
    FILE *file;
    size_t index;
    size_t count;
    if (framebuffer == NULL || framebuffer->pixels == NULL || path == NULL) {
        return PET_STATUS_INVALID_ARGUMENT;
    }
    file = fopen(path, "wb");
    if (file == NULL) {
        return PET_STATUS_IO_ERROR;
    }
    (void)fprintf(file, "P6\n%d %d\n255\n", framebuffer->width, framebuffer->height);
    count = (size_t)framebuffer->width * (size_t)framebuffer->height;
    for (index = 0U; index < count; index++) {
        uint16_t pixel = framebuffer->pixels[index];
        unsigned char rgb[3] = {
            (unsigned char)(((pixel >> 11U) & 0x1FU) * 255U / 31U),
            (unsigned char)(((pixel >> 5U) & 0x3FU) * 255U / 63U),
            (unsigned char)((pixel & 0x1FU) * 255U / 31U)
        };
        if (fwrite(rgb, sizeof(rgb), 1U, file) != 1U) {
            (void)fclose(file);
            return PET_STATUS_IO_ERROR;
        }
    }
    return fclose(file) == 0 ? PET_STATUS_OK : PET_STATUS_IO_ERROR;
}

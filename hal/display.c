#include "hal/display.h"

static pet_status_t validate(const pet_display_t *display)
{
    return display == NULL || display->ops == NULL ? PET_STATUS_INVALID_ARGUMENT
                                                   : PET_STATUS_OK;
}

pet_status_t pet_display_init(pet_display_t *display)
{
    if (validate(display) != PET_STATUS_OK || display->ops->init == NULL) {
        return PET_STATUS_NOT_SUPPORTED;
    }
    return display->ops->init(display->context);
}

pet_status_t pet_display_clear(pet_display_t *display, uint16_t color)
{
    if (validate(display) != PET_STATUS_OK || display->ops->clear == NULL) {
        return PET_STATUS_NOT_SUPPORTED;
    }
    return display->ops->clear(display->context, color);
}

pet_status_t pet_display_draw_pixel(pet_display_t *display, int x, int y, uint16_t color)
{
    if (validate(display) != PET_STATUS_OK || display->ops->draw_pixel == NULL) {
        return PET_STATUS_NOT_SUPPORTED;
    }
    return display->ops->draw_pixel(display->context, x, y, color);
}

pet_status_t pet_display_draw_bitmap(pet_display_t *display, int x, int y, int width,
                                     int height, const uint16_t *pixels)
{
    if (validate(display) != PET_STATUS_OK || display->ops->draw_bitmap == NULL ||
        pixels == NULL || width <= 0 || height <= 0) {
        return PET_STATUS_INVALID_ARGUMENT;
    }
    return display->ops->draw_bitmap(display->context, x, y, width, height, pixels);
}

pet_status_t pet_display_draw_region(pet_display_t *display, int x, int y, int width,
                                     int height, const uint16_t *pixels,
                                     size_t stride_pixels)
{
    if (validate(display) != PET_STATUS_OK || display->ops->draw_region == NULL ||
        pixels == NULL || width <= 0 || height <= 0 || stride_pixels < (size_t)width) {
        return PET_STATUS_INVALID_ARGUMENT;
    }
    return display->ops->draw_region(display->context, x, y, width, height, pixels,
                                     stride_pixels);
}

pet_status_t pet_display_flush(pet_display_t *display)
{
    if (validate(display) != PET_STATUS_OK || display->ops->flush == NULL) {
        return PET_STATUS_NOT_SUPPORTED;
    }
    return display->ops->flush(display->context);
}

pet_status_t pet_display_set_rotation(pet_display_t *display, uint8_t rotation)
{
    if (rotation > 3U || validate(display) != PET_STATUS_OK ||
        display->ops->set_rotation == NULL) {
        return PET_STATUS_INVALID_ARGUMENT;
    }
    return display->ops->set_rotation(display->context, rotation);
}

int pet_display_width(const pet_display_t *display)
{
    return validate(display) == PET_STATUS_OK && display->ops->width != NULL
               ? display->ops->width(display->context)
               : 0;
}

int pet_display_height(const pet_display_t *display)
{
    return validate(display) == PET_STATUS_OK && display->ops->height != NULL
               ? display->ops->height(display->context)
               : 0;
}

pet_status_t pet_display_set_brightness(pet_display_t *display, uint8_t percent)
{
    if (percent > 100U || validate(display) != PET_STATUS_OK ||
        display->ops->set_brightness == NULL) {
        return PET_STATUS_INVALID_ARGUMENT;
    }
    return display->ops->set_brightness(display->context, percent);
}

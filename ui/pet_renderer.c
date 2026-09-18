#include "ui/pet_renderer.h"

#include "animation/pet_assets.h"

static pet_status_t fill_circle(pet_display_t *display, int cx, int cy, int radius,
                                uint16_t color)
{
    int x;
    int y;
    for (y = -radius; y <= radius; y++) {
        for (x = -radius; x <= radius; x++) {
            if (x * x + y * y <= radius * radius) {
                pet_status_t status = pet_display_draw_pixel(display, cx + x, cy + y, color);
                if (status != PET_STATUS_OK) {
                    return status;
                }
            }
        }
    }
    return PET_STATUS_OK;
}

static pet_status_t draw_line(pet_display_t *display, int x0, int y0, int x1, int y1,
                              uint16_t color)
{
    int dx = x1 > x0 ? x1 - x0 : x0 - x1;
    int sx = x0 < x1 ? 1 : -1;
    int dy_abs = y1 > y0 ? y1 - y0 : y0 - y1;
    int dy = -dy_abs;
    int sy = y0 < y1 ? 1 : -1;
    int error = dx + dy;

    for (;;) {
        int error2;
        pet_status_t status = pet_display_draw_pixel(display, x0, y0, color);
        if (status != PET_STATUS_OK) {
            return status;
        }
        if (x0 == x1 && y0 == y1) {
            break;
        }
        error2 = 2 * error;
        if (error2 >= dy) {
            error += dy;
            x0 += sx;
        }
        if (error2 <= dx) {
            error += dx;
            y0 += sy;
        }
    }
    return PET_STATUS_OK;
}

static pet_status_t draw_mouth(pet_renderer_t *renderer, int cx, int cy, bool happy)
{
    pet_status_t status;
    if (happy) {
        status = draw_line(renderer->display, cx - 20, cy, cx - 10, cy + 9,
                           renderer->theme.feature_color);
        if (status == PET_STATUS_OK) {
            status = draw_line(renderer->display, cx - 10, cy + 9, cx, cy + 12,
                               renderer->theme.feature_color);
        }
        if (status == PET_STATUS_OK) {
            status = draw_line(renderer->display, cx, cy + 12, cx + 10, cy + 9,
                               renderer->theme.feature_color);
        }
        if (status == PET_STATUS_OK) {
            status = draw_line(renderer->display, cx + 10, cy + 9, cx + 20, cy,
                               renderer->theme.feature_color);
        }
        return status;
    } else {
        return draw_line(renderer->display, cx - 9, cy + 6, cx + 9, cy + 6,
                         renderer->theme.feature_color);
    }
}

static pet_status_t draw_bitmap_asset(pet_renderer_t *renderer, int display_width,
                                      int display_height, pet_asset_id_t asset_id)
{
    pet_bitmap_t bitmap;
    pet_status_t status;
    size_t scale_x;
    size_t scale_y;
    size_t scale;
    int origin_x;
    int origin_y;
    size_t source_x;
    size_t source_y;
    size_t pixel_x;
    size_t pixel_y;

    if (renderer->assets == NULL || renderer->assets->get_bitmap == NULL) {
        return PET_STATUS_NOT_SUPPORTED;
    }
    status = renderer->assets->get_bitmap(renderer->assets->context, asset_id, &bitmap);
    if (status != PET_STATUS_OK) {
        return status;
    }
    if (bitmap.pixels == NULL || bitmap.width == 0U || bitmap.height == 0U ||
        bitmap.stride_pixels < bitmap.width) {
        status = PET_STATUS_INVALID_ARGUMENT;
        goto release;
    }

    scale_x = (size_t)display_width / bitmap.width;
    scale_y = (size_t)display_height / bitmap.height;
    scale = scale_x < scale_y ? scale_x : scale_y;
    if (scale == 0U) {
        scale = 1U;
    }
    origin_x = (display_width - (int)(bitmap.width * scale)) / 2;
    origin_y = (display_height - (int)(bitmap.height * scale)) / 2;
    for (source_y = 0U; source_y < bitmap.height; source_y++) {
        for (source_x = 0U; source_x < bitmap.width; source_x++) {
            uint16_t color = bitmap.pixels[source_y * bitmap.stride_pixels + source_x];
            for (pixel_y = 0U; pixel_y < scale; pixel_y++) {
                for (pixel_x = 0U; pixel_x < scale; pixel_x++) {
                    status = pet_display_draw_pixel(
                        renderer->display,
                        origin_x + (int)(source_x * scale + pixel_x),
                        origin_y + (int)(source_y * scale + pixel_y), color);
                    if (status != PET_STATUS_OK) {
                        goto release;
                    }
                }
            }
        }
    }
release:
    if (renderer->assets->release_bitmap != NULL) {
        renderer->assets->release_bitmap(renderer->assets->context, &bitmap);
    }
    return status;
}

pet_status_t pet_renderer_init(pet_renderer_t *renderer, pet_display_t *display,
                               const pet_renderer_theme_t *theme)
{
    if (renderer == NULL || display == NULL || theme == NULL) {
        return PET_STATUS_INVALID_ARGUMENT;
    }
    renderer->display = display;
    renderer->assets = NULL;
    renderer->theme = *theme;
    return PET_STATUS_OK;
}

void pet_renderer_set_asset_provider(pet_renderer_t *renderer,
                                     const pet_asset_provider_t *assets)
{
    if (renderer != NULL) {
        renderer->assets = assets;
    }
}

pet_status_t pet_renderer_render(pet_renderer_t *renderer,
                                 const pet_app_snapshot_t *snapshot)
{
    int width;
    int height;
    int cx;
    int cy;
    int eye_y;
    int eye_offset = 0;
    bool closed = false;
    bool half = false;
    bool happy = false;
    pet_asset_id_t asset;
    pet_status_t status;

    if (renderer == NULL || renderer->display == NULL || snapshot == NULL ||
        snapshot->frame == NULL) {
        return PET_STATUS_INVALID_ARGUMENT;
    }
    width = pet_display_width(renderer->display);
    height = pet_display_height(renderer->display);
    if (width <= 0 || height <= 0) {
        return PET_STATUS_NOT_INITIALIZED;
    }
    cx = width / 2;
    cy = height / 2;
    eye_y = cy - 15;
    asset = snapshot->frame->asset_id;

    status = pet_display_clear(renderer->display, renderer->theme.background_color);
    if (status != PET_STATUS_OK) {
        return status;
    }
    status = draw_bitmap_asset(renderer, width, height, asset);
    if (status == PET_STATUS_OK) {
        return pet_display_flush(renderer->display);
    }
    if (status != PET_STATUS_NOT_SUPPORTED) {
        return status;
    }

    closed = asset == PET_ASSET_BLINK_CLOSED || asset == PET_ASSET_SLEEP_0 ||
             asset == PET_ASSET_SLEEP_1;
    half = asset == PET_ASSET_BLINK_HALF;
    happy = asset == PET_ASSET_HAPPY_0 || asset == PET_ASSET_HAPPY_1;
    if (asset == PET_ASSET_LOOK_LEFT) {
        eye_offset = -8;
    } else if (asset == PET_ASSET_LOOK_RIGHT) {
        eye_offset = 8;
    }

    status = fill_circle(renderer->display, cx, cy,
                         width < height ? width / 3 : height / 3,
                         renderer->theme.face_color);
    if (status != PET_STATUS_OK) {
        return status;
    }
    if (closed || half) {
        status = draw_line(renderer->display, cx - 42, eye_y, cx - 18, eye_y,
                           renderer->theme.feature_color);
        if (status == PET_STATUS_OK) {
            status = draw_line(renderer->display, cx + 18, eye_y, cx + 42, eye_y,
                               renderer->theme.feature_color);
        }
        if (half) {
            if (status == PET_STATUS_OK) {
                status = draw_line(renderer->display, cx - 40, eye_y + 2,
                                   cx - 20, eye_y + 2,
                                   renderer->theme.feature_color);
            }
            if (status == PET_STATUS_OK) {
                status = draw_line(renderer->display, cx + 20, eye_y + 2,
                                   cx + 40, eye_y + 2,
                                   renderer->theme.feature_color);
            }
        }
    } else {
        status = fill_circle(renderer->display, cx - 30 + eye_offset, eye_y, 8,
                             renderer->theme.feature_color);
        if (status == PET_STATUS_OK) {
            status = fill_circle(renderer->display, cx + 30 + eye_offset, eye_y, 8,
                                 renderer->theme.feature_color);
        }
    }
    if (status != PET_STATUS_OK) {
        return status;
    }
    status = draw_mouth(renderer, cx, cy + 20, happy);
    if (status != PET_STATUS_OK) {
        return status;
    }
    return pet_display_flush(renderer->display);
}

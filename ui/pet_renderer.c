#include "ui/pet_renderer.h"

#include "animation/pet_assets.h"

static void fill_circle(pet_display_t *display, int cx, int cy, int radius, uint16_t color)
{
    int x;
    int y;
    for (y = -radius; y <= radius; y++) {
        for (x = -radius; x <= radius; x++) {
            if (x * x + y * y <= radius * radius) {
                (void)pet_display_draw_pixel(display, cx + x, cy + y, color);
            }
        }
    }
}

static void draw_line(pet_display_t *display, int x0, int y0, int x1, int y1,
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
        (void)pet_display_draw_pixel(display, x0, y0, color);
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
}

static void draw_mouth(pet_renderer_t *renderer, int cx, int cy, bool happy)
{
    if (happy) {
        draw_line(renderer->display, cx - 20, cy, cx - 10, cy + 9,
                  renderer->theme.feature_color);
        draw_line(renderer->display, cx - 10, cy + 9, cx, cy + 12,
                  renderer->theme.feature_color);
        draw_line(renderer->display, cx, cy + 12, cx + 10, cy + 9,
                  renderer->theme.feature_color);
        draw_line(renderer->display, cx + 10, cy + 9, cx + 20, cy,
                  renderer->theme.feature_color);
    } else {
        draw_line(renderer->display, cx - 9, cy + 6, cx + 9, cy + 6,
                  renderer->theme.feature_color);
    }
}

pet_status_t pet_renderer_init(pet_renderer_t *renderer, pet_display_t *display,
                               const pet_renderer_theme_t *theme)
{
    if (renderer == NULL || display == NULL || theme == NULL) {
        return PET_STATUS_INVALID_ARGUMENT;
    }
    renderer->display = display;
    renderer->theme = *theme;
    return PET_STATUS_OK;
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

    closed = asset == PET_ASSET_BLINK_CLOSED || asset == PET_ASSET_SLEEP_0 ||
             asset == PET_ASSET_SLEEP_1;
    half = asset == PET_ASSET_BLINK_HALF;
    happy = asset == PET_ASSET_HAPPY_0 || asset == PET_ASSET_HAPPY_1;
    if (asset == PET_ASSET_LOOK_LEFT) {
        eye_offset = -8;
    } else if (asset == PET_ASSET_LOOK_RIGHT) {
        eye_offset = 8;
    }

    (void)pet_display_clear(renderer->display, renderer->theme.background_color);
    fill_circle(renderer->display, cx, cy, width < height ? width / 3 : height / 3,
                renderer->theme.face_color);
    if (closed || half) {
        draw_line(renderer->display, cx - 42, eye_y, cx - 18, eye_y,
                  renderer->theme.feature_color);
        draw_line(renderer->display, cx + 18, eye_y, cx + 42, eye_y,
                  renderer->theme.feature_color);
        if (half) {
            draw_line(renderer->display, cx - 40, eye_y + 2, cx - 20, eye_y + 2,
                      renderer->theme.feature_color);
            draw_line(renderer->display, cx + 20, eye_y + 2, cx + 40, eye_y + 2,
                      renderer->theme.feature_color);
        }
    } else {
        fill_circle(renderer->display, cx - 30 + eye_offset, eye_y, 8,
                    renderer->theme.feature_color);
        fill_circle(renderer->display, cx + 30 + eye_offset, eye_y, 8,
                    renderer->theme.feature_color);
    }
    draw_mouth(renderer, cx, cy + 20, happy);
    return pet_display_flush(renderer->display);
}

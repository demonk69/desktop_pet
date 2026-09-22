#include "ui/pet_renderer.h"

#include "animation/pet_assets.h"
#include "services/pet_clock_format.h"

#define CLOCK_SCALE       3
#define CLOCK_GLYPH_W     5
#define CLOCK_GLYPH_H     7
#define CLOCK_CHAR_GAP    1
#define CLOCK_TOP_Y       8
#define CLOCK_GLYPH_COUNT 12
#define SCENE_LABEL_SCALE 4

/* 5x7 glyphs: digits 0..9, ':' (index 10), '-' (index 11). Bit 4 is the
 * leftmost pixel of each row. */
static const uint8_t clock_glyphs[CLOCK_GLYPH_COUNT][CLOCK_GLYPH_H] = {
    { 0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E },
    { 0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E },
    { 0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F },
    { 0x1F, 0x02, 0x04, 0x02, 0x01, 0x11, 0x0E },
    { 0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02 },
    { 0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E },
    { 0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E },
    { 0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08 },
    { 0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E },
    { 0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x0C },
    { 0x00, 0x0C, 0x0C, 0x00, 0x0C, 0x0C, 0x00 },
    { 0x00, 0x00, 0x00, 0x0E, 0x00, 0x00, 0x00 },
};

static uint8_t clock_glyph_index(char character)
{
    if (character >= '0' && character <= '9') {
        return (uint8_t)(character - '0');
    }
    if (character == ':') {
        return 10;
    }
    return 11;
}

static uint8_t text_glyph_row(char character, int row)
{
    if (character >= 'a' && character <= 'z') {
        character = (char)(character - 'a' + 'A');
    }
    if (character >= '0' && character <= '9') {
        return clock_glyphs[clock_glyph_index(character)][row];
    }
    if (character == ':' || character == '-') {
        return clock_glyphs[clock_glyph_index(character)][row];
    }
    switch (character) {
    case 'A': return (uint8_t[]){ 0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11 }[row];
    case 'C': return (uint8_t[]){ 0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E }[row];
    case 'E': return (uint8_t[]){ 0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F }[row];
    case 'G': return (uint8_t[]){ 0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0E }[row];
    case 'H': return (uint8_t[]){ 0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11 }[row];
    case 'I': return (uint8_t[]){ 0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E }[row];
    case 'K': return (uint8_t[]){ 0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11 }[row];
    case 'L': return (uint8_t[]){ 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F }[row];
    case 'M': return (uint8_t[]){ 0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11 }[row];
    case 'N': return (uint8_t[]){ 0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11 }[row];
    case 'O': return (uint8_t[]){ 0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E }[row];
    case 'S': return (uint8_t[]){ 0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E }[row];
    case 'T': return (uint8_t[]){ 0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04 }[row];
    case 'U': return (uint8_t[]){ 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E }[row];
    default: return 0x00;
    }
}

static pet_status_t draw_text_centered(pet_renderer_t *renderer, int width,
                                       int top_y, int scale, uint16_t color,
                                       const char *text)
{
    int text_chars = 0;
    int origin_x;
    int index;
    pet_status_t status = PET_STATUS_OK;

    if (text == NULL || scale <= 0) {
        return PET_STATUS_INVALID_ARGUMENT;
    }
    while (text[text_chars] != '\0') {
        text_chars++;
    }
    origin_x = (width - (text_chars * CLOCK_GLYPH_W * scale +
                         (text_chars - 1) * CLOCK_CHAR_GAP * scale)) /
               2;
    for (index = 0; index < text_chars && status == PET_STATUS_OK; index++) {
        int glyph_x = origin_x + index * (CLOCK_GLYPH_W + CLOCK_CHAR_GAP) * scale;
        int row;
        int column;
        for (row = 0; row < CLOCK_GLYPH_H && status == PET_STATUS_OK; row++) {
            uint8_t bits = text_glyph_row(text[index], row);
            for (column = 0; column < CLOCK_GLYPH_W; column++) {
                int block_x;
                int block_y;
                if ((bits & (0x10U >> column)) == 0U) {
                    continue;
                }
                for (block_y = 0; block_y < scale; block_y++) {
                    for (block_x = 0; block_x < scale; block_x++) {
                        status = pet_display_draw_pixel(
                            renderer->display, glyph_x + column * scale + block_x,
                            top_y + row * scale + block_y, color);
                        if (status != PET_STATUS_OK) {
                            break;
                        }
                    }
                    if (status != PET_STATUS_OK) {
                        break;
                    }
                }
            }
        }
    }
    return status;
}

static const char *scene_label(pet_ui_scene_id_t scene)
{
    switch (scene) {
    case PET_UI_SCENE_HOME:
        return "HOME";
    case PET_UI_SCENE_CLOCK:
        return "CLOCK";
    case PET_UI_SCENE_STATUS:
        return "STATUS";
    case PET_UI_SCENE_SETTINGS:
        return "SETTINGS";
    case PET_UI_SCENE_COUNT:
    default:
        return "SCENE";
    }
}

static void clock_text_from_snapshot(const pet_time_snapshot_t *time_snapshot,
                                     char output[6])
{
    if (time_snapshot != NULL && time_snapshot->valid &&
        time_snapshot->hour < 24U && time_snapshot->minute < 60U) {
        pet_clock_format_hhmm(time_snapshot->hour, time_snapshot->minute,
                              output);
    } else {
        pet_clock_format_unknown(output);
    }
}

static pet_status_t draw_scene_label_page(pet_renderer_t *renderer, int width,
                                          int height, pet_ui_scene_id_t scene)
{
    pet_status_t status = pet_display_clear(renderer->display,
                                            renderer->theme.background_color);
    if (status == PET_STATUS_OK) {
        status = draw_text_centered(renderer, width,
                                    (height - CLOCK_GLYPH_H * SCENE_LABEL_SCALE) / 2,
                                    SCENE_LABEL_SCALE,
                                    renderer->theme.feature_color,
                                    scene_label(scene));
    }
    return status == PET_STATUS_OK ? pet_display_flush(renderer->display) : status;
}

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
            if (bitmap.has_transparent_color && color == bitmap.transparent_color) {
                continue;
            }
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
    pet_ui_scene_id_t visible_scene;
    bool clock_entered;
    char clock_text[6];

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
    visible_scene = pet_ui_snapshot_visible_scene(&snapshot->ui);
    clock_entered = snapshot->ui.entered &&
                    snapshot->ui.current == PET_UI_SCENE_CLOCK;

    if (visible_scene != PET_UI_SCENE_HOME &&
        visible_scene != PET_UI_SCENE_COUNT && !clock_entered) {
        return draw_scene_label_page(renderer, width, height, visible_scene);
    }

    status = pet_display_clear(renderer->display, renderer->theme.background_color);
    if (status != PET_STATUS_OK) {
        return status;
    }
    status = draw_bitmap_asset(renderer, width, height, asset);
    if (status == PET_STATUS_OK) {
        const char *overlay_text = scene_label(visible_scene);
        if (clock_entered) {
            clock_text_from_snapshot(&snapshot->ui.time_snapshot, clock_text);
            overlay_text = clock_text;
        }
        status = draw_text_centered(renderer, width, CLOCK_TOP_Y, CLOCK_SCALE,
                                    renderer->theme.feature_color, overlay_text);
        if (status != PET_STATUS_OK) {
            return status;
        }
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
    {
        const char *overlay_text = scene_label(visible_scene);
        if (clock_entered) {
            clock_text_from_snapshot(&snapshot->ui.time_snapshot, clock_text);
            overlay_text = clock_text;
        }
        status = draw_text_centered(renderer, width, CLOCK_TOP_Y, CLOCK_SCALE,
                                    renderer->theme.feature_color, overlay_text);
    }
    if (status != PET_STATUS_OK) {
        return status;
    }
    return pet_display_flush(renderer->display);
}

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "animation/pet_assets.h"
#include "config/pet_config.h"
#include "file_asset_provider.h"
#include "framebuffer_display.h"
#include "pet/pet_app.h"
#include "services/pet_time_service.h"
#include "ui/pet_renderer.h"

#ifndef PET_TEST_ASSET_ROOT
#define PET_TEST_ASSET_ROOT "assets/pet"
#endif

#define TEST_CLOCK_SCALE    3
#define TEST_CLOCK_GLYPH_W  5
#define TEST_CLOCK_GLYPH_H  7
#define TEST_CLOCK_CHAR_GAP 1
#define TEST_CLOCK_TOP_Y    8

#define TEST_TRANSPARENT_ASSET_ID 9001U
#define TEST_TRANSPARENT_SIZE     64U
#define TEST_TRANSPARENT_COLOR    0x0001U
#define TEST_SPRITE_COLOR         0xF800U
#define TEST_SPRITE_MARK_X        10U
#define TEST_SPRITE_MARK_Y        10U

#define TEST_CHARACTER_SIZE          64U
#define TEST_CHARACTER_FACE_COLOR     0xFEC0U
#define TEST_CHARACTER_ALT_FACE_COLOR 0xFFE0U
#define TEST_CHARACTER_FEATURE_COLOR  0x20E5U

typedef struct {
    uint16_t pixels[TEST_TRANSPARENT_SIZE * TEST_TRANSPARENT_SIZE];
    bool released;
} test_transparent_asset_t;

static uint32_t framebuffer_hash(const framebuffer_display_t *framebuffer)
{
    uint32_t hash = 2166136261U;
    size_t index;
    size_t count = (size_t)framebuffer->width * (size_t)framebuffer->height;
    for (index = 0U; index < count; index++) {
        hash ^= framebuffer->pixels[index];
        hash *= 16777619U;
    }
    return hash;
}

static bool clock_band_has_color(const framebuffer_display_t *framebuffer,
                                  uint16_t color)
{
    int x;
    int y;
    for (y = 8; y <= 28; y++) {
        for (x = 0; x < framebuffer->width; x++) {
            if (framebuffer->pixels[(size_t)y * framebuffer->width + (size_t)x] ==
                color) {
                return true;
            }
        }
    }
    return false;
}

static size_t test_text_length(const char *text)
{
    size_t length = 0U;
    while (text[length] != '\0') {
        length++;
    }
    return length;
}

static uint8_t test_text_glyph_row(char character, int row)
{
    switch (character) {
    case '1': return (uint8_t[]){ 0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E }[row];
    case '2': return (uint8_t[]){ 0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F }[row];
    case '3': return (uint8_t[]){ 0x1F, 0x02, 0x04, 0x02, 0x01, 0x11, 0x0E }[row];
    case '4': return (uint8_t[]){ 0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02 }[row];
    case ':': return (uint8_t[]){ 0x00, 0x0C, 0x0C, 0x00, 0x0C, 0x0C, 0x00 }[row];
    case '-': return (uint8_t[]){ 0x00, 0x00, 0x00, 0x0E, 0x00, 0x00, 0x00 }[row];
    default: return 0x00;
    }
}

static bool expected_text_pixel_on(const char *text, int x, int y, int scale)
{
    int pitch = (TEST_CLOCK_GLYPH_W + TEST_CLOCK_CHAR_GAP) * scale;
    int char_index = x / pitch;
    int char_x = x % pitch;
    int column;
    uint8_t row_bits;

    if ((size_t)char_index >= test_text_length(text) ||
        char_x >= TEST_CLOCK_GLYPH_W * scale) {
        return false;
    }
    column = char_x / scale;
    row_bits = test_text_glyph_row(text[char_index], y / scale);
    return (row_bits & (uint8_t)(0x10U >> column)) != 0U;
}

static void assert_centered_clock_text(const framebuffer_display_t *framebuffer,
                                       uint16_t background_color,
                                       uint16_t feature_color,
                                       const char *text)
{
    size_t text_chars = test_text_length(text);
    int text_width = (int)(text_chars * TEST_CLOCK_GLYPH_W * TEST_CLOCK_SCALE +
                           (text_chars - 1U) * TEST_CLOCK_CHAR_GAP *
                               TEST_CLOCK_SCALE);
    int origin_x = (framebuffer->width - text_width) / 2;
    int height = TEST_CLOCK_GLYPH_H * TEST_CLOCK_SCALE;
    int x;
    int y;

    for (y = 0; y < height; y++) {
        for (x = 0; x < text_width; x++) {
            uint16_t pixel = framebuffer->pixels[(size_t)(TEST_CLOCK_TOP_Y + y) *
                                                 (size_t)framebuffer->width +
                                                 (size_t)(origin_x + x)];
            assert(pixel == (expected_text_pixel_on(text, x, y, TEST_CLOCK_SCALE)
                                 ? feature_color
                                 : background_color));
        }
    }
}

static void init_transparent_asset(test_transparent_asset_t *asset)
{
    size_t index;
    for (index = 0U; index < TEST_TRANSPARENT_SIZE * TEST_TRANSPARENT_SIZE;
         index++) {
        asset->pixels[index] = TEST_TRANSPARENT_COLOR;
    }
    asset->pixels[TEST_SPRITE_MARK_Y * TEST_TRANSPARENT_SIZE +
                  TEST_SPRITE_MARK_X] = TEST_SPRITE_COLOR;
    asset->released = false;
}

static pet_status_t get_transparent_bitmap(void *context,
                                           pet_asset_id_t asset_id,
                                           pet_bitmap_t *bitmap)
{
    test_transparent_asset_t *asset = context;
    if (asset == NULL || bitmap == NULL) {
        return PET_STATUS_INVALID_ARGUMENT;
    }
    if (asset_id != TEST_TRANSPARENT_ASSET_ID) {
        return PET_STATUS_NOT_SUPPORTED;
    }
    *bitmap = (pet_bitmap_t){ .pixels = asset->pixels,
                              .width = TEST_TRANSPARENT_SIZE,
                              .height = TEST_TRANSPARENT_SIZE,
                              .stride_pixels = TEST_TRANSPARENT_SIZE,
                              .has_transparent_color = true,
                              .transparent_color = TEST_TRANSPARENT_COLOR };
    return PET_STATUS_OK;
}

static void release_transparent_bitmap(void *context, const pet_bitmap_t *bitmap)
{
    test_transparent_asset_t *asset = context;
    (void)bitmap;
    if (asset != NULL) {
        asset->released = true;
    }
}

static void assert_transparent_sprite_rendered(
    const framebuffer_display_t *framebuffer, uint16_t background_color)
{
    int scale = framebuffer->width / (int)TEST_TRANSPARENT_SIZE;
    int scale_y = framebuffer->height / (int)TEST_TRANSPARENT_SIZE;
    int origin_x;
    int origin_y;
    uint16_t transparent_pixel;
    uint16_t color_pixel;

    if (scale_y < scale) {
        scale = scale_y;
    }
    assert(scale > 0);
    origin_x = (framebuffer->width - (int)TEST_TRANSPARENT_SIZE * scale) / 2;
    origin_y = (framebuffer->height - (int)TEST_TRANSPARENT_SIZE * scale) / 2;
    transparent_pixel = framebuffer->pixels[
        (size_t)(origin_y + ((int)TEST_TRANSPARENT_SIZE - 1) * scale) *
            (size_t)framebuffer->width +
        (size_t)origin_x];
    color_pixel = framebuffer->pixels[
        (size_t)(origin_y + (int)TEST_SPRITE_MARK_Y * scale) *
            (size_t)framebuffer->width +
        (size_t)(origin_x + (int)TEST_SPRITE_MARK_X * scale)];

    assert(transparent_pixel == background_color);
    assert(color_pixel == TEST_SPRITE_COLOR);
}

static uint16_t sprite_pixel_at_source(const framebuffer_display_t *framebuffer,
                                       size_t sprite_size, size_t source_x,
                                       size_t source_y)
{
    int scale = framebuffer->width / (int)sprite_size;
    int scale_y = framebuffer->height / (int)sprite_size;
    int origin_x;
    int origin_y;

    if (scale_y < scale) {
        scale = scale_y;
    }
    assert(scale > 0);
    origin_x = (framebuffer->width - (int)sprite_size * scale) / 2;
    origin_y = (framebuffer->height - (int)sprite_size * scale) / 2;
    return framebuffer->pixels[(size_t)(origin_y + (int)source_y * scale) *
                                   (size_t)framebuffer->width +
                               (size_t)(origin_x + (int)source_x * scale)];
}

static void assert_test_character_rendered(const framebuffer_display_t *framebuffer,
                                           uint16_t background_color,
                                           uint16_t face_color)
{
    assert(sprite_pixel_at_source(framebuffer, TEST_CHARACTER_SIZE, 0U, 0U) ==
           background_color);
    assert(sprite_pixel_at_source(framebuffer, TEST_CHARACTER_SIZE, 32U, 20U) ==
           face_color);
    assert(sprite_pixel_at_source(framebuffer, TEST_CHARACTER_SIZE, 32U, 38U) ==
           TEST_CHARACTER_FEATURE_COLOR);
}

int main(void)
{
    pet_config_t config;
    pet_app_t app;
    pet_display_t display;
    framebuffer_display_t framebuffer;
    pet_file_asset_provider_t assets;
    pet_renderer_t renderer;
    pet_app_snapshot_t snapshot;
    pet_renderer_theme_t theme = { 0x18C3U, 0xFEC0U, 0x2104U, 0xF9A6U };
    pet_event_t event;
    uint32_t boot_hash;
    uint32_t idle_hash;
    uint32_t idle_alt_hash;
    pet_time_snapshot_t fake_time;
    test_transparent_asset_t transparent_asset;
    pet_asset_provider_t transparent_provider;
    static const pet_animation_frame_t transparent_frame = {
        TEST_TRANSPARENT_ASSET_ID, 100U
    };

    pet_config_set_development_defaults(&config);
    config.app.behavior.auto_blink_min_interval_ms = 3000U;
    config.app.behavior.auto_blink_max_interval_ms = 3000U;
    config.app.behavior.idle_look_inactivity_ms = 60000U;
    config.app.behavior.idle_look_min_interval_ms = 60000U;
    config.app.behavior.idle_look_max_interval_ms = 60000U;
    assert(pet_app_init(&app, &config.app) == PET_STATUS_OK);
    assert(framebuffer_display_create(&framebuffer, &display, config.hardware.width,
                                      config.hardware.height) == PET_STATUS_OK);
    assert(pet_display_init(&display) == PET_STATUS_OK);
    assert(pet_file_asset_provider_init(&assets, PET_TEST_ASSET_ROOT, "manifest.txt") ==
           PET_STATUS_OK);
    assert(pet_renderer_init(&renderer, &display, &theme) == PET_STATUS_OK);
    pet_renderer_set_asset_provider(&renderer, pet_file_asset_provider_interface(&assets));

    snapshot = pet_app_snapshot(&app);
    assert(pet_renderer_render(&renderer, &snapshot) == PET_STATUS_OK);
    boot_hash = framebuffer_hash(&framebuffer);

    pet_app_update(&app, config.app.core.boot_duration_ms);
    snapshot = pet_app_snapshot(&app);
    assert(snapshot.frame->asset_id == PET_ASSET_TEST_CHARACTER);
    assert(pet_renderer_render(&renderer, &snapshot) == PET_STATUS_OK);
    idle_hash = framebuffer_hash(&framebuffer);
    assert(idle_hash != boot_hash);
    assert_test_character_rendered(&framebuffer, theme.background_color,
                                   TEST_CHARACTER_FACE_COLOR);

    pet_app_update(&app, 700U);
    snapshot = pet_app_snapshot(&app);
    assert(snapshot.state == PET_STATE_IDLE);
    assert(snapshot.frame->asset_id == PET_ASSET_TEST_CHARACTER_IDLE_1);
    assert(pet_renderer_render(&renderer, &snapshot) == PET_STATUS_OK);
    idle_alt_hash = framebuffer_hash(&framebuffer);
    assert(idle_alt_hash != idle_hash);
    assert_test_character_rendered(&framebuffer, theme.background_color,
                                   TEST_CHARACTER_ALT_FACE_COLOR);

    pet_app_update(&app, 700U);
    snapshot = pet_app_snapshot(&app);
    assert(snapshot.state == PET_STATE_IDLE);
    assert(snapshot.frame->asset_id == PET_ASSET_TEST_CHARACTER);
    assert(pet_renderer_render(&renderer, &snapshot) == PET_STATUS_OK);
    assert(framebuffer_hash(&framebuffer) == idle_hash);

    event = (pet_event_t){ .type = PET_EVENT_LOOK,
                           .timestamp_ms = app.now_ms,
                           .data.look_direction = PET_LOOK_LEFT };
    assert(pet_app_post_event(&app, &event));
    pet_app_update(&app, 0U);
    snapshot = pet_app_snapshot(&app);
    assert(pet_renderer_render(&renderer, &snapshot) == PET_STATUS_OK);
    assert_test_character_rendered(&framebuffer, theme.background_color,
                                   TEST_CHARACTER_FACE_COLOR);

    pet_app_update(&app, 620U);
    assert(pet_app_snapshot(&app).state == PET_STATE_IDLE);
    pet_app_update(&app, config.app.behavior.auto_blink_min_interval_ms);
    snapshot = pet_app_snapshot(&app);
    assert(snapshot.state == PET_STATE_BLINK);
    assert(snapshot.frame->asset_id == PET_ASSET_TEST_CHARACTER_BLINK_HALF);
    assert(pet_renderer_render(&renderer, &snapshot) == PET_STATUS_OK);
    assert_test_character_rendered(&framebuffer, theme.background_color,
                                   TEST_CHARACTER_FACE_COLOR);

    pet_app_update(&app, 80U);
    snapshot = pet_app_snapshot(&app);
    assert(snapshot.state == PET_STATE_BLINK);
    assert(snapshot.frame->asset_id == PET_ASSET_TEST_CHARACTER_BLINK_CLOSED);
    assert(pet_renderer_render(&renderer, &snapshot) == PET_STATUS_OK);
    assert_test_character_rendered(&framebuffer, theme.background_color,
                                   TEST_CHARACTER_FACE_COLOR);

    pet_app_update(&app, 260U);
    snapshot = pet_app_snapshot(&app);
    assert(snapshot.state == PET_STATE_IDLE);
    assert(snapshot.animation == PET_ANIM_IDLE);
    assert(pet_renderer_render(&renderer, &snapshot) == PET_STATUS_OK);
    assert(framebuffer_hash(&framebuffer) == idle_hash);

    /* Scene label: HOME is overlaid on the pet, while selected scenes render a
     * full label page for rotary navigation bring-up. */
    {
        uint32_t home_scene_hash;
        uint32_t clock_scene_hash;
        uint32_t home_again_hash;

        snapshot = pet_app_snapshot(&app);
        assert(pet_renderer_render(&renderer, &snapshot) == PET_STATUS_OK);
        assert(clock_band_has_color(&framebuffer, theme.feature_color));
        home_scene_hash = framebuffer_hash(&framebuffer);

        event = (pet_event_t){ .type = PET_EVENT_NAV_NEXT,
                               .timestamp_ms = app.now_ms };
        assert(pet_app_post_event(&app, &event));
        pet_app_update(&app, 0U);
        snapshot = pet_app_snapshot(&app);
        assert(snapshot.ui.current == PET_UI_SCENE_HOME);
        assert(snapshot.ui.selected == PET_UI_SCENE_CLOCK);
        assert(pet_renderer_render(&renderer, &snapshot) == PET_STATUS_OK);
        clock_scene_hash = framebuffer_hash(&framebuffer);
        assert(clock_scene_hash != home_scene_hash);

        event = (pet_event_t){ .type = PET_EVENT_BUTTON,
                               .timestamp_ms = app.now_ms };
        assert(pet_app_post_event(&app, &event));
        pet_app_update(&app, 0U);
        fake_time = (pet_time_snapshot_t){ .valid = true, .hour = 12,
                                           .minute = 34, .second = 0 };
        pet_app_set_time_snapshot(&app, &fake_time);
        snapshot = pet_app_snapshot(&app);
        assert(snapshot.ui.current == PET_UI_SCENE_CLOCK);
        assert(snapshot.ui.entered);
        assert(snapshot.ui.time_snapshot.valid);
        assert(snapshot.ui.time_snapshot.hour == 12U);
        assert(snapshot.ui.time_snapshot.minute == 34U);
        pet_renderer_set_asset_provider(&renderer, NULL);
        assert(pet_renderer_render(&renderer, &snapshot) == PET_STATUS_OK);
        assert_centered_clock_text(&framebuffer, theme.background_color,
                                   theme.feature_color, "12:34");

        fake_time = (pet_time_snapshot_t){ .valid = false };
        pet_app_set_time_snapshot(&app, &fake_time);
        snapshot = pet_app_snapshot(&app);
        assert(pet_renderer_render(&renderer, &snapshot) == PET_STATUS_OK);
        assert_centered_clock_text(&framebuffer, theme.background_color,
                                   theme.feature_color, "--:--");

        event = (pet_event_t){ .type = PET_EVENT_BUTTON,
                               .timestamp_ms = app.now_ms };
        assert(pet_app_post_event(&app, &event));
        pet_app_update(&app, 0U);
        snapshot = pet_app_snapshot(&app);
        assert(snapshot.ui.current == PET_UI_SCENE_HOME);
        assert(snapshot.ui.selected == PET_UI_SCENE_HOME);
        assert(pet_renderer_render(&renderer, &snapshot) == PET_STATUS_OK);
        home_again_hash = framebuffer_hash(&framebuffer);
        assert(home_again_hash != clock_scene_hash);
    }

    init_transparent_asset(&transparent_asset);
    transparent_provider = (pet_asset_provider_t){ &transparent_asset,
                                                   get_transparent_bitmap,
                                                   release_transparent_bitmap };
    snapshot = pet_app_snapshot(&app);
    snapshot.frame = &transparent_frame;
    snapshot.ui = (pet_ui_snapshot_t){ .current = PET_UI_SCENE_HOME,
                                       .selected = PET_UI_SCENE_HOME,
                                       .entered = false };
    pet_renderer_set_asset_provider(&renderer, &transparent_provider);
    assert(pet_renderer_render(&renderer, &snapshot) == PET_STATUS_OK);
    assert(transparent_asset.released);
    assert_transparent_sprite_rendered(&framebuffer, theme.background_color);

    pet_file_asset_provider_destroy(&assets);
    framebuffer_display_destroy(&framebuffer);
    (void)printf("test_renderer_integration: ok\n");
    return 0;
}

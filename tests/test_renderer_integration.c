#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "animation/pet_assets.h"
#include "config/pet_config.h"
#include "file_asset_provider.h"
#include "framebuffer_display.h"
#include "pet/pet_app.h"
#include "ui/pet_renderer.h"

#ifndef PET_TEST_ASSET_ROOT
#define PET_TEST_ASSET_ROOT "assets/pet"
#endif

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
    uint32_t look_hash;
    uint32_t blink_half_hash;
    uint32_t blink_closed_hash;

    pet_config_set_development_defaults(&config);
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
    assert(pet_renderer_render(&renderer, &snapshot) == PET_STATUS_OK);
    idle_hash = framebuffer_hash(&framebuffer);
    assert(idle_hash != boot_hash);

    event = (pet_event_t){ .type = PET_EVENT_LOOK,
                           .timestamp_ms = app.now_ms,
                           .data.look_direction = PET_LOOK_LEFT };
    assert(pet_app_post_event(&app, &event));
    pet_app_update(&app, 0U);
    snapshot = pet_app_snapshot(&app);
    assert(pet_renderer_render(&renderer, &snapshot) == PET_STATUS_OK);
    look_hash = framebuffer_hash(&framebuffer);
    assert(look_hash != idle_hash);

    pet_app_update(&app, 620U);
    assert(pet_app_snapshot(&app).state == PET_STATE_IDLE);
    pet_app_update(&app, config.app.core.idle_action_interval_ms);
    snapshot = pet_app_snapshot(&app);
    assert(snapshot.state == PET_STATE_BLINK);
    assert(snapshot.frame->asset_id == PET_ASSET_BLINK_HALF);
    assert(pet_renderer_render(&renderer, &snapshot) == PET_STATUS_OK);
    blink_half_hash = framebuffer_hash(&framebuffer);

    pet_app_update(&app, 80U);
    snapshot = pet_app_snapshot(&app);
    assert(snapshot.state == PET_STATE_BLINK);
    assert(snapshot.frame->asset_id == PET_ASSET_BLINK_CLOSED);
    assert(pet_renderer_render(&renderer, &snapshot) == PET_STATUS_OK);
    blink_closed_hash = framebuffer_hash(&framebuffer);
    assert(blink_closed_hash != blink_half_hash);

    pet_app_update(&app, 260U);
    snapshot = pet_app_snapshot(&app);
    assert(snapshot.state == PET_STATE_IDLE);
    assert(snapshot.animation == PET_ANIM_IDLE);
    assert(pet_renderer_render(&renderer, &snapshot) == PET_STATUS_OK);
    assert(framebuffer_hash(&framebuffer) == idle_hash);

    pet_file_asset_provider_destroy(&assets);
    framebuffer_display_destroy(&framebuffer);
    (void)printf("test_renderer_integration: ok\n");
    return 0;
}

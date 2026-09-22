#ifndef PET_ASSET_PROVIDER_H
#define PET_ASSET_PROVIDER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "animation/pet_animation.h"
#include "pet/status.h"

typedef struct {
    const uint16_t *pixels;
    size_t width;
    size_t height;
    size_t stride_pixels;
    bool has_transparent_color;
    uint16_t transparent_color;
} pet_bitmap_t;

typedef struct {
    void *context;
    pet_status_t (*get_bitmap)(void *context, pet_asset_id_t asset_id,
                               pet_bitmap_t *bitmap);
    void (*release_bitmap)(void *context, const pet_bitmap_t *bitmap);
} pet_asset_provider_t;

#endif

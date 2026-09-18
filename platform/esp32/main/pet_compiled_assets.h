#ifndef PET_COMPILED_ASSETS_H
#define PET_COMPILED_ASSETS_H

#include <stdint.h>

#include "animation/pet_asset_provider.h"
#include "animation/pet_assets.h"

#define PET_COMPILED_ASSET_WIDTH  6U
#define PET_COMPILED_ASSET_HEIGHT 6U

typedef struct {
    pet_asset_provider_t interface;
    uint16_t pixels[PET_ASSET_IDLE_1 + 1U]
                   [PET_COMPILED_ASSET_WIDTH * PET_COMPILED_ASSET_HEIGHT];
} pet_compiled_asset_provider_t;

pet_status_t pet_compiled_asset_provider_init(pet_compiled_asset_provider_t *provider);
const pet_asset_provider_t *pet_compiled_asset_provider_interface(
    pet_compiled_asset_provider_t *provider);

#endif

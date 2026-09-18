#ifndef PET_ASSETS_H
#define PET_ASSETS_H

#include "animation/pet_animation.h"

/* Logical asset keys. Storage backends may resolve these to Flash, PSRAM, or files. */
enum {
    PET_ASSET_BOOT_0 = 1,
    PET_ASSET_BOOT_1,
    PET_ASSET_IDLE,
    PET_ASSET_BLINK_HALF,
    PET_ASSET_BLINK_CLOSED,
    PET_ASSET_LOOK_LEFT,
    PET_ASSET_LOOK_RIGHT,
    PET_ASSET_HAPPY_0,
    PET_ASSET_HAPPY_1,
    PET_ASSET_SLEEP_0,
    PET_ASSET_SLEEP_1,
    PET_ASSET_IDLE_1
};

#endif

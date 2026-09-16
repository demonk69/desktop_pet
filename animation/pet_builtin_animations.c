#include "animation/pet_animation.h"
#include "animation/pet_assets.h"

#define FRAME(asset, duration) { (asset), (duration) }

static const pet_animation_frame_t boot_frames[] = {
    FRAME(PET_ASSET_BOOT_0, 300U), FRAME(PET_ASSET_BOOT_1, 300U)
};
static const pet_animation_frame_t idle_frames[] = { FRAME(PET_ASSET_IDLE, 1000U) };
static const pet_animation_frame_t blink_frames[] = {
    FRAME(PET_ASSET_BLINK_HALF, 80U), FRAME(PET_ASSET_BLINK_CLOSED, 100U),
    FRAME(PET_ASSET_BLINK_HALF, 80U), FRAME(PET_ASSET_IDLE, 80U)
};
static const pet_animation_frame_t look_left_frames[] = {
    FRAME(PET_ASSET_LOOK_LEFT, 500U), FRAME(PET_ASSET_IDLE, 120U)
};
static const pet_animation_frame_t look_right_frames[] = {
    FRAME(PET_ASSET_LOOK_RIGHT, 500U), FRAME(PET_ASSET_IDLE, 120U)
};
static const pet_animation_frame_t happy_frames[] = {
    FRAME(PET_ASSET_HAPPY_0, 180U), FRAME(PET_ASSET_HAPPY_1, 180U),
    FRAME(PET_ASSET_HAPPY_0, 180U), FRAME(PET_ASSET_HAPPY_1, 180U)
};
static const pet_animation_frame_t sleep_frames[] = {
    FRAME(PET_ASSET_SLEEP_0, 700U), FRAME(PET_ASSET_SLEEP_1, 700U)
};

static const pet_animation_clip_t clips[PET_ANIM_COUNT] = {
    [PET_ANIM_BOOT] = { boot_frames, 2U, false },
    [PET_ANIM_IDLE] = { idle_frames, 1U, true },
    [PET_ANIM_BLINK] = { blink_frames, 4U, false },
    [PET_ANIM_LOOK_LEFT] = { look_left_frames, 2U, false },
    [PET_ANIM_LOOK_RIGHT] = { look_right_frames, 2U, false },
    [PET_ANIM_HAPPY] = { happy_frames, 4U, false },
    [PET_ANIM_SLEEP] = { sleep_frames, 2U, true }
};

static const pet_animation_clip_t *get_builtin_clip(void *context, pet_animation_id_t id)
{
    (void)context;
    return id < PET_ANIM_COUNT ? &clips[id] : NULL;
}

const pet_animation_catalog_t *pet_builtin_animation_catalog(void)
{
    static const pet_animation_catalog_t catalog = { NULL, get_builtin_clip };
    return &catalog;
}

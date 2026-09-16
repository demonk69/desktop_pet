#include <assert.h>
#include <stdio.h>

#include "animation/pet_animation.h"

static const pet_animation_frame_t frames[] = {
    { 10U, 100U }, { 11U, 200U }
};
static const pet_animation_clip_t clip = { frames, 2U, false };
static const pet_animation_clip_t loop_clip = { frames, 2U, true };

static const pet_animation_clip_t *get_clip(void *context, pet_animation_id_t id)
{
    (void)context;
    if (id == PET_ANIM_IDLE) {
        return &loop_clip;
    }
    return id == PET_ANIM_BLINK ? &clip : NULL;
}

int main(void)
{
    pet_animation_catalog_t catalog = { NULL, get_clip };
    pet_animation_player_t player;
    pet_animation_id_t completed;

    assert(pet_animation_player_init(&player, &catalog) == PET_STATUS_OK);
    assert(pet_play_animation(&player, PET_ANIM_BLINK) == PET_STATUS_OK);
    assert(pet_animation_current_frame(&player)->asset_id == 10U);
    pet_animation_update(&player, 100U);
    assert(pet_animation_current_frame(&player)->asset_id == 11U);
    pet_animation_update(&player, 200U);
    assert(pet_animation_take_completed(&player, &completed));
    assert(completed == PET_ANIM_BLINK);
    assert(!pet_animation_take_completed(&player, &completed));

    assert(pet_play_animation(&player, PET_ANIM_IDLE) == PET_STATUS_OK);
    pet_animation_update(&player, 300U);
    assert(pet_animation_current_frame(&player)->asset_id == 10U);
    assert(!pet_animation_take_completed(&player, NULL));
    assert(pet_play_animation(&player, PET_ANIM_HAPPY) == PET_STATUS_NOT_SUPPORTED);
    assert(pet_play_animation(&player, PET_ANIM_COUNT) == PET_STATUS_INVALID_ARGUMENT);

    (void)printf("test_animation: ok\n");
    return 0;
}

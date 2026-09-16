#include "animation/pet_animation.h"

pet_status_t pet_animation_player_init(pet_animation_player_t *player,
                                       const pet_animation_catalog_t *catalog)
{
    if (player == NULL || catalog == NULL || catalog->get_clip == NULL) {
        return PET_STATUS_INVALID_ARGUMENT;
    }
    player->catalog = catalog;
    player->clip = NULL;
    player->current_id = PET_ANIM_COUNT;
    player->frame_index = 0U;
    player->frame_elapsed_ms = 0U;
    player->playing = false;
    player->completion_pending = false;
    return PET_STATUS_OK;
}

pet_status_t pet_play_animation(pet_animation_player_t *player, pet_animation_id_t id)
{
    const pet_animation_clip_t *clip;
    size_t index;

    if (player == NULL || player->catalog == NULL || id >= PET_ANIM_COUNT) {
        return PET_STATUS_INVALID_ARGUMENT;
    }
    clip = player->catalog->get_clip(player->catalog->context, id);
    if (clip == NULL || clip->frames == NULL || clip->frame_count == 0U) {
        return PET_STATUS_NOT_SUPPORTED;
    }
    for (index = 0U; index < clip->frame_count; index++) {
        if (clip->frames[index].duration_ms == 0U) {
            return PET_STATUS_INVALID_ARGUMENT;
        }
    }

    player->clip = clip;
    player->current_id = id;
    player->frame_index = 0U;
    player->frame_elapsed_ms = 0U;
    player->playing = true;
    player->completion_pending = false;
    return PET_STATUS_OK;
}

void pet_animation_update(pet_animation_player_t *player, uint32_t delta_ms)
{
    if (player == NULL || !player->playing || player->clip == NULL) {
        return;
    }

    player->frame_elapsed_ms += delta_ms;
    while (player->playing &&
           player->frame_elapsed_ms >= player->clip->frames[player->frame_index].duration_ms) {
        player->frame_elapsed_ms -= player->clip->frames[player->frame_index].duration_ms;
        if (player->frame_index + 1U < player->clip->frame_count) {
            player->frame_index++;
        } else if (player->clip->loop) {
            player->frame_index = 0U;
        } else {
            player->playing = false;
            player->completion_pending = true;
            player->frame_elapsed_ms = 0U;
        }
    }
}

const pet_animation_frame_t *pet_animation_current_frame(const pet_animation_player_t *player)
{
    if (player == NULL || player->clip == NULL) {
        return NULL;
    }
    return &player->clip->frames[player->frame_index];
}

pet_animation_id_t pet_animation_current_id(const pet_animation_player_t *player)
{
    return player == NULL ? PET_ANIM_COUNT : player->current_id;
}

bool pet_animation_take_completed(pet_animation_player_t *player,
                                  pet_animation_id_t *completed_id)
{
    if (player == NULL || !player->completion_pending) {
        return false;
    }
    if (completed_id != NULL) {
        *completed_id = player->current_id;
    }
    player->completion_pending = false;
    return true;
}

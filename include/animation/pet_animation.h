#ifndef PET_ANIMATION_H
#define PET_ANIMATION_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "pet/status.h"

typedef enum {
    PET_ANIM_BOOT = 0,
    PET_ANIM_IDLE,
    PET_ANIM_BLINK,
    PET_ANIM_LOOK_LEFT,
    PET_ANIM_LOOK_RIGHT,
    PET_ANIM_HAPPY,
    PET_ANIM_SLEEP,
    PET_ANIM_COUNT
} pet_animation_id_t;

typedef uint32_t pet_asset_id_t;

typedef struct {
    pet_asset_id_t asset_id;
    uint32_t duration_ms;
} pet_animation_frame_t;

typedef struct {
    const pet_animation_frame_t *frames;
    size_t frame_count;
    bool loop;
} pet_animation_clip_t;

typedef struct {
    void *context;
    const pet_animation_clip_t *(*get_clip)(void *context, pet_animation_id_t id);
} pet_animation_catalog_t;

typedef struct {
    const pet_animation_catalog_t *catalog;
    const pet_animation_clip_t *clip;
    pet_animation_id_t current_id;
    size_t frame_index;
    uint32_t frame_elapsed_ms;
    bool playing;
    bool completion_pending;
} pet_animation_player_t;

pet_status_t pet_animation_player_init(pet_animation_player_t *player,
                                       const pet_animation_catalog_t *catalog);
pet_status_t pet_play_animation(pet_animation_player_t *player, pet_animation_id_t id);
void pet_animation_update(pet_animation_player_t *player, uint32_t delta_ms);
const pet_animation_frame_t *pet_animation_current_frame(const pet_animation_player_t *player);
pet_animation_id_t pet_animation_current_id(const pet_animation_player_t *player);
bool pet_animation_take_completed(pet_animation_player_t *player,
                                  pet_animation_id_t *completed_id);

const pet_animation_catalog_t *pet_builtin_animation_catalog(void);

#endif

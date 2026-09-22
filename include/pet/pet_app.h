#ifndef PET_APP_H
#define PET_APP_H

#include <stdbool.h>
#include <stdint.h>

#include "animation/pet_animation.h"
#include "behavior/pet_behavior.h"
#include "core/pet_event_queue.h"
#include "pet/pet_core.h"
#include "pet/status.h"
#include "ui/pet_scene.h"

typedef struct {
    pet_core_config_t core;
    pet_behavior_config_t behavior;
    const pet_animation_catalog_t *animations;
} pet_app_config_t;

typedef struct {
    pet_state_t state;
    pet_animation_id_t animation;
    const pet_animation_frame_t *frame;
    pet_ui_snapshot_t ui;
} pet_app_snapshot_t;

typedef struct {
    pet_core_t core;
    pet_core_config_t core_config;
    pet_behavior_manager_t behavior;
    pet_animation_player_t animation;
    pet_scene_manager_t scene;
    pet_event_queue_t events;
    uint32_t now_ms;
    uint32_t last_user_activity_ms;
} pet_app_t;

pet_status_t pet_app_init(pet_app_t *app, const pet_app_config_t *config);
bool pet_app_post_event(pet_app_t *app, const pet_event_t *event);
void pet_app_set_time_snapshot(pet_app_t *app,
                               const pet_time_snapshot_t *snapshot);
void pet_app_update(pet_app_t *app, uint32_t delta_ms);
pet_app_snapshot_t pet_app_snapshot(const pet_app_t *app);

#endif

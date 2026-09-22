#ifndef PET_SCENE_H
#define PET_SCENE_H

#include <stdbool.h>

#include "core/pet_event.h"
#include "pet/status.h"
#include "services/pet_time_service.h"

typedef enum {
    PET_UI_SCENE_HOME = 0,
    PET_UI_SCENE_CLOCK,
    PET_UI_SCENE_STATUS,
    PET_UI_SCENE_SETTINGS,
    PET_UI_SCENE_COUNT
} pet_ui_scene_id_t;

typedef struct {
    pet_ui_scene_id_t current;
    pet_ui_scene_id_t selected;
    bool entered;
    pet_time_snapshot_t time_snapshot;
} pet_ui_snapshot_t;

typedef struct {
    pet_ui_snapshot_t snapshot;
} pet_scene_manager_t;

pet_status_t pet_scene_manager_init(pet_scene_manager_t *manager);
bool pet_scene_manager_handle_event(pet_scene_manager_t *manager,
                                    const pet_event_t *event);
void pet_scene_manager_set_time_snapshot(pet_scene_manager_t *manager,
                                         const pet_time_snapshot_t *snapshot);
pet_ui_snapshot_t pet_scene_manager_get_state(const pet_scene_manager_t *manager);
pet_ui_scene_id_t pet_ui_snapshot_visible_scene(const pet_ui_snapshot_t *snapshot);
const char *pet_ui_scene_name(pet_ui_scene_id_t scene);

#endif

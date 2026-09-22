#include "ui/pet_scene.h"

#include <stddef.h>
#include <stdio.h>

static pet_ui_scene_id_t next_selectable_scene(pet_ui_scene_id_t scene)
{
    switch (scene) {
    case PET_UI_SCENE_HOME:
        return PET_UI_SCENE_CLOCK;
    case PET_UI_SCENE_CLOCK:
        return PET_UI_SCENE_STATUS;
    case PET_UI_SCENE_STATUS:
        return PET_UI_SCENE_SETTINGS;
    case PET_UI_SCENE_SETTINGS:
    case PET_UI_SCENE_COUNT:
    default:
        return PET_UI_SCENE_HOME;
    }
}

static pet_ui_scene_id_t previous_selectable_scene(pet_ui_scene_id_t scene)
{
    switch (scene) {
    case PET_UI_SCENE_HOME:
        return PET_UI_SCENE_SETTINGS;
    case PET_UI_SCENE_CLOCK:
        return PET_UI_SCENE_HOME;
    case PET_UI_SCENE_STATUS:
        return PET_UI_SCENE_CLOCK;
    case PET_UI_SCENE_SETTINGS:
        return PET_UI_SCENE_STATUS;
    case PET_UI_SCENE_COUNT:
    default:
        return PET_UI_SCENE_HOME;
    }
}

static const char *scene_event_name(pet_event_type_t type)
{
    switch (type) {
    case PET_EVENT_NAV_NEXT:
        return "NAV_NEXT";
    case PET_EVENT_NAV_PREV:
        return "NAV_PREV";
    case PET_EVENT_BUTTON:
        return "BUTTON";
    default:
        return "OTHER";
    }
}

static bool should_log_scene_event(pet_event_type_t type)
{
    return type == PET_EVENT_NAV_NEXT || type == PET_EVENT_NAV_PREV ||
           type == PET_EVENT_BUTTON;
}

static void log_scene_event(const pet_scene_manager_t *manager,
                            const pet_event_t *event, bool consumed)
{
    if (manager == NULL || event == NULL || !should_log_scene_event(event->type)) {
        return;
    }
    (void)printf("SCENE EVENT: event=%s current=%s selected=%s entered=%d consumed=%d\n",
                 scene_event_name(event->type),
                 pet_ui_scene_name(manager->snapshot.current),
                 pet_ui_scene_name(manager->snapshot.selected),
                 manager->snapshot.entered ? 1 : 0, consumed ? 1 : 0);
}

pet_status_t pet_scene_manager_init(pet_scene_manager_t *manager)
{
    if (manager == NULL) {
        return PET_STATUS_INVALID_ARGUMENT;
    }
    manager->snapshot = (pet_ui_snapshot_t){ .current = PET_UI_SCENE_HOME,
                                             .selected = PET_UI_SCENE_HOME,
                                             .entered = false };
    return PET_STATUS_OK;
}

bool pet_scene_manager_handle_event(pet_scene_manager_t *manager,
                                    const pet_event_t *event)
{
    bool consumed = false;
    if (manager == NULL || event == NULL) {
        return false;
    }

    if (manager->snapshot.current == PET_UI_SCENE_HOME) {
        switch (event->type) {
        case PET_EVENT_NAV_NEXT:
            manager->snapshot.selected =
                next_selectable_scene(manager->snapshot.selected);
            consumed = true;
            break;
        case PET_EVENT_NAV_PREV:
            manager->snapshot.selected =
                previous_selectable_scene(manager->snapshot.selected);
            consumed = true;
            break;
        case PET_EVENT_BUTTON:
            if (manager->snapshot.selected != PET_UI_SCENE_HOME) {
                manager->snapshot.current = manager->snapshot.selected;
                manager->snapshot.entered = true;
                consumed = true;
            }
            break;
        default:
            break;
        }
    } else {
        switch (event->type) {
        case PET_EVENT_BUTTON:
            manager->snapshot.current = PET_UI_SCENE_HOME;
            manager->snapshot.selected = PET_UI_SCENE_HOME;
            manager->snapshot.entered = false;
            consumed = true;
            break;
        case PET_EVENT_NAV_NEXT:
        case PET_EVENT_NAV_PREV:
            consumed = true;
            break;
        default:
            break;
        }
    }
    log_scene_event(manager, event, consumed);
    return consumed;
}

void pet_scene_manager_set_time_snapshot(pet_scene_manager_t *manager,
                                         const pet_time_snapshot_t *snapshot)
{
    if (manager != NULL && snapshot != NULL) {
        manager->snapshot.time_snapshot = *snapshot;
    }
}

pet_ui_snapshot_t pet_scene_manager_get_state(const pet_scene_manager_t *manager)
{
    return manager == NULL ? (pet_ui_snapshot_t){ .current = PET_UI_SCENE_COUNT,
                                                  .selected = PET_UI_SCENE_COUNT,
                                                  .entered = false }
                           : manager->snapshot;
}

pet_ui_scene_id_t pet_ui_snapshot_visible_scene(const pet_ui_snapshot_t *snapshot)
{
    if (snapshot == NULL) {
        return PET_UI_SCENE_COUNT;
    }
    return snapshot->entered ? snapshot->current : snapshot->selected;
}

const char *pet_ui_scene_name(pet_ui_scene_id_t scene)
{
    static const char *const names[PET_UI_SCENE_COUNT] = {
        "home", "clock", "status", "settings"
    };
    return scene < PET_UI_SCENE_COUNT ? names[scene] : "invalid";
}

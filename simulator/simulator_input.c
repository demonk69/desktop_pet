#include "simulator_input.h"

bool simulator_input_translate(const SDL_Event *platform_event, uint32_t timestamp_ms,
                               pet_event_t *pet_event, bool *quit_requested)
{
    if (platform_event == NULL || pet_event == NULL || quit_requested == NULL) {
        return false;
    }
    *quit_requested = false;
    *pet_event = (pet_event_t){ .type = PET_EVENT_NONE, .timestamp_ms = timestamp_ms };
    if (platform_event->type == SDL_QUIT) {
        *quit_requested = true;
        return false;
    }
    if (platform_event->type != SDL_KEYDOWN || platform_event->key.repeat != 0U) {
        return false;
    }

    switch (platform_event->key.keysym.sym) {
    case SDLK_ESCAPE:
        *quit_requested = true;
        return false;
    case SDLK_SPACE:
        pet_event->type = PET_EVENT_BUTTON;
        pet_event->data.button_id = 0;
        return true;
    case SDLK_h:
        pet_event->type = PET_EVENT_HAPPY;
        return true;
    case SDLK_s:
        pet_event->type = PET_EVENT_SLEEP;
        return true;
    case SDLK_w:
        pet_event->type = PET_EVENT_WAKE;
        return true;
    case SDLK_m:
        pet_event->type = PET_EVENT_MESSAGE;
        pet_event->data.message_id = 1U;
        return true;
    case SDLK_LEFT:
        pet_event->type = PET_EVENT_NAV_PREV;
        return true;
    case SDLK_RIGHT:
        pet_event->type = PET_EVENT_NAV_NEXT;
        return true;
    default:
        return false;
    }
}

const char *simulator_event_name(const pet_event_t *event)
{
    if (event == NULL) {
        return "INVALID";
    }
    switch (event->type) {
    case PET_EVENT_BUTTON:
        return "BUTTON";
    case PET_EVENT_HAPPY:
        return "HAPPY";
    case PET_EVENT_BLINK:
        return "BLINK";
    case PET_EVENT_SLEEP:
        return "SLEEP";
    case PET_EVENT_WAKE:
        return "WAKE";
    case PET_EVENT_MESSAGE:
        return "MESSAGE id=1 (Hello Pet)";
    case PET_EVENT_LOOK:
        return event->data.look_direction == PET_LOOK_LEFT ? "LOOK LEFT" : "LOOK RIGHT";
    case PET_EVENT_NAV_NEXT:
        return "NAV NEXT";
    case PET_EVENT_NAV_PREV:
        return "NAV PREV";
    default:
        return "OTHER";
    }
}

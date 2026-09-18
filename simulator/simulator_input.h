#ifndef PET_SIMULATOR_INPUT_H
#define PET_SIMULATOR_INPUT_H

#include <stdbool.h>
#include <stdint.h>

#include <SDL.h>

#include "core/pet_event.h"

bool simulator_input_translate(const SDL_Event *platform_event, uint32_t timestamp_ms,
                               pet_event_t *pet_event, bool *quit_requested);
const char *simulator_event_name(const pet_event_t *event);

#endif

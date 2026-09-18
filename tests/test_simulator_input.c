#include <assert.h>
#include <stdio.h>

#include <SDL.h>

#include "simulator_input.h"

static pet_event_t translate(SDL_Keycode key, bool *quit)
{
    SDL_Event input = { 0 };
    pet_event_t output;
    input.type = SDL_KEYDOWN;
    input.key.keysym.sym = key;
    assert(simulator_input_translate(&input, 42U, &output, quit) || *quit);
    assert(output.timestamp_ms == 42U);
    return output;
}

int main(void)
{
    pet_event_t event;
    bool quit;

    event = translate(SDLK_SPACE, &quit);
    assert(!quit && event.type == PET_EVENT_BUTTON);
    event = translate(SDLK_h, &quit);
    assert(!quit && event.type == PET_EVENT_HAPPY);
    event = translate(SDLK_s, &quit);
    assert(!quit && event.type == PET_EVENT_SLEEP);
    event = translate(SDLK_w, &quit);
    assert(!quit && event.type == PET_EVENT_WAKE);
    event = translate(SDLK_m, &quit);
    assert(!quit && event.type == PET_EVENT_MESSAGE && event.data.message_id == 1U);
    event = translate(SDLK_LEFT, &quit);
    assert(!quit && event.type == PET_EVENT_LOOK &&
           event.data.look_direction == PET_LOOK_LEFT);
    event = translate(SDLK_RIGHT, &quit);
    assert(!quit && event.type == PET_EVENT_LOOK &&
           event.data.look_direction == PET_LOOK_RIGHT);
    (void)translate(SDLK_ESCAPE, &quit);
    assert(quit);

    (void)printf("test_simulator_input: ok\n");
    return 0;
}

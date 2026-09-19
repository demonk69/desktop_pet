#ifndef PET_ESP32_RUNTIME_H
#define PET_ESP32_RUNTIME_H

#include "hal/backlight.h"
#include "pet/pet_app.h"
#include "pet_esp32_display.h"
#include "pet_esp32_rotary.h"
#include "ui/pet_renderer.h"

pet_status_t pet_esp32_runtime_run(pet_app_t *app, pet_renderer_t *renderer,
                                    pet_esp32_display_t *display_backend,
                                    pet_backlight_t *backlight,
                                    pet_esp32_rotary_t *rotary);

#endif

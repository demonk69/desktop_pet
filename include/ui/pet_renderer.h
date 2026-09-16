#ifndef PET_RENDERER_H
#define PET_RENDERER_H

#include <stdint.h>

#include "hal/display.h"
#include "pet/pet_app.h"
#include "pet/status.h"

typedef struct {
    uint16_t background_color;
    uint16_t face_color;
    uint16_t feature_color;
    uint16_t accent_color;
} pet_renderer_theme_t;

typedef struct {
    pet_display_t *display;
    pet_renderer_theme_t theme;
} pet_renderer_t;

pet_status_t pet_renderer_init(pet_renderer_t *renderer, pet_display_t *display,
                               const pet_renderer_theme_t *theme);
pet_status_t pet_renderer_render(pet_renderer_t *renderer,
                                 const pet_app_snapshot_t *snapshot);

#endif

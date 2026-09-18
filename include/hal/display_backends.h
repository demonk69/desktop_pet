#ifndef PET_DISPLAY_BACKENDS_H
#define PET_DISPLAY_BACKENDS_H

#include "config/pet_config.h"
#include "hal/display.h"

/* Generic unsupported stubs; real platform factories live with their platform target. */
pet_status_t pet_display_create_spi_stub(pet_display_t *display,
                                         const pet_hardware_config_t *config);
pet_status_t pet_display_create_i8080_stub(pet_display_t *display,
                                           const pet_hardware_config_t *config);
pet_status_t pet_display_create_st7789_stub(pet_display_t *display,
                                            const pet_hardware_config_t *config);

#endif

#ifndef PET_ESP32_DIAG_H
#define PET_ESP32_DIAG_H

#include <stdbool.h>

#include "hal/display.h"
#include "pet/status.h"

/* Draws the fixed diagnostic pattern once, flushes it once, then either keeps
 * re-flushing the unchanged framebuffer at 10 Hz (repeat) or never touches the
 * display again (once). Never returns while the board is powered. */
pet_status_t pet_esp32_diag_run_static(pet_display_t *display, bool repeat_flush);

#endif

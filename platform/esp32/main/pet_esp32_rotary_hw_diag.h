#ifndef PET_ESP32_ROTARY_HW_DIAG_H
#define PET_ESP32_ROTARY_HW_DIAG_H

#include <stdbool.h>

#include "pet/status.h"

const char *pet_esp32_rotary_diag_mode_name(void);
bool pet_esp32_rotary_diag_mode_is_valid(void);
bool pet_esp32_rotary_hw_diag_enabled(void);
pet_status_t pet_esp32_rotary_hw_diag_run(void);
bool pet_esp32_rotary_adc_margin_enabled(void);
pet_status_t pet_esp32_rotary_adc_margin_run(void);

#endif

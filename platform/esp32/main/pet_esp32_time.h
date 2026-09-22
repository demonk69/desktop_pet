#ifndef PET_ESP32_TIME_H
#define PET_ESP32_TIME_H

#include <stdint.h>

#include "pet/status.h"

pet_status_t pet_esp32_time_init_timezone(void);
uint64_t pet_esp32_time_now_us(void);

#endif

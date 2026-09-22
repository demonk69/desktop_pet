#include "pet_esp32_time.h"

#include <stdlib.h>
#include <time.h>

#include "esp_timer.h"

#if !defined(PET_TIMEZONE)
#define PET_TIMEZONE "CST-8"
#endif

pet_status_t pet_esp32_time_init_timezone(void)
{
    if (setenv("TZ", PET_TIMEZONE, 1) != 0) {
        return PET_STATUS_NO_MEMORY;
    }
    tzset();
    return PET_STATUS_OK;
}

uint64_t pet_esp32_time_now_us(void)
{
    return (uint64_t)esp_timer_get_time();
}

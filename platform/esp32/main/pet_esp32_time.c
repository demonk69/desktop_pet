#include "pet_esp32_time.h"

#include "esp_timer.h"

uint64_t pet_esp32_time_now_us(void)
{
    return (uint64_t)esp_timer_get_time();
}

#include "services/pet_clock_format.h"

#include <stddef.h>

void pet_clock_format_hhmm(uint8_t hour, uint8_t minute, char output[6])
{
    if (output == NULL) {
        return;
    }
    output[0] = (char)('0' + (hour / 10U) % 10U);
    output[1] = (char)('0' + hour % 10U);
    output[2] = ':';
    output[3] = (char)('0' + (minute / 10U) % 10U);
    output[4] = (char)('0' + minute % 10U);
    output[5] = '\0';
}

void pet_clock_format_unknown(char output[6])
{
    if (output == NULL) {
        return;
    }
    output[0] = '-';
    output[1] = '-';
    output[2] = ':';
    output[3] = '-';
    output[4] = '-';
    output[5] = '\0';
}

#ifndef PET_CLOCK_FORMAT_H
#define PET_CLOCK_FORMAT_H

#include <stdint.h>

/* Formats hour/minute as "HH:MM" (hour 0..23, minute 0..59). */
void pet_clock_format_hhmm(uint8_t hour, uint8_t minute, char output[6]);

/* Formats the unknown-time placeholder "--:--". */
void pet_clock_format_unknown(char output[6]);

#endif

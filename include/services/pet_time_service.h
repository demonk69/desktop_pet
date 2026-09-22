#ifndef PET_TIME_SERVICE_H
#define PET_TIME_SERVICE_H

#include <stdbool.h>
#include <stdint.h>

#include "pet/status.h"

/* Times before 2021-01-01 are treated as invalid. The ESP32 system clock starts
 * at 0 on boot, so this threshold marks "time synchronized" without coupling
 * the shared layer to SNTP. */
#define PET_TIME_VALID_MIN_EPOCH 1609459200LL

typedef struct {
    bool valid;
    uint8_t hour;   /* 0..23 */
    uint8_t minute; /* 0..59 */
    uint8_t second; /* 0..59 */
} pet_time_snapshot_t;

typedef struct {
    pet_status_t (*get_snapshot)(void *context, pet_time_snapshot_t *snapshot);
} pet_time_service_ops_t;

typedef struct {
    void *context;
    const pet_time_service_ops_t *ops;
} pet_time_service_t;

/* System-clock backend: time() + localtime_r, respecting the TZ environment.
 * The caller (platform init) is responsible for setting TZ before use. */
pet_status_t pet_time_service_init_system(pet_time_service_t *service);
pet_status_t pet_time_service_get_snapshot(const pet_time_service_t *service,
                                           pet_time_snapshot_t *snapshot);

#endif

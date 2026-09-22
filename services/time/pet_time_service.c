#define _POSIX_C_SOURCE 200809L

#include "services/pet_time_service.h"

#include <stddef.h>
#include <time.h>

static pet_status_t system_get_snapshot(void *context, pet_time_snapshot_t *snapshot)
{
    time_t now = time(NULL);
    struct tm time_info;
    (void)context;
    if (snapshot == NULL) {
        return PET_STATUS_INVALID_ARGUMENT;
    }
    *snapshot = (pet_time_snapshot_t){ 0 };
    if (now < (time_t)PET_TIME_VALID_MIN_EPOCH ||
        localtime_r(&now, &time_info) == NULL) {
        return PET_STATUS_OK;
    }
    snapshot->valid = true;
    snapshot->hour = (uint8_t)time_info.tm_hour;
    snapshot->minute = (uint8_t)time_info.tm_min;
    snapshot->second = (uint8_t)time_info.tm_sec;
    return PET_STATUS_OK;
}

static const pet_time_service_ops_t system_ops = { system_get_snapshot };

pet_status_t pet_time_service_init_system(pet_time_service_t *service)
{
    if (service == NULL) {
        return PET_STATUS_INVALID_ARGUMENT;
    }
    service->context = NULL;
    service->ops = &system_ops;
    return PET_STATUS_OK;
}

pet_status_t pet_time_service_get_snapshot(const pet_time_service_t *service,
                                           pet_time_snapshot_t *snapshot)
{
    if (service == NULL || service->ops == NULL || service->ops->get_snapshot == NULL) {
        return PET_STATUS_INVALID_ARGUMENT;
    }
    return service->ops->get_snapshot(service->context, snapshot);
}

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "services/pet_clock_format.h"
#include "services/pet_network.h"
#include "services/pet_time_service.h"

typedef struct {
    pet_time_snapshot_t snapshot;
} fake_time_context_t;

typedef struct {
    pet_network_state_t state;
} fake_network_context_t;

static pet_status_t fake_get_snapshot(void *context, pet_time_snapshot_t *snapshot)
{
    fake_time_context_t *fake = context;
    if (fake == NULL || snapshot == NULL) {
        return PET_STATUS_INVALID_ARGUMENT;
    }
    *snapshot = fake->snapshot;
    return PET_STATUS_OK;
}

static pet_network_state_t fake_get_network_state(void *context)
{
    fake_network_context_t *fake = context;
    if (fake == NULL) {
        return PET_NETWORK_ERROR;
    }
    return fake->state;
}

int main(void)
{
    static const pet_time_service_ops_t fake_ops = { fake_get_snapshot };
    pet_time_service_t service;
    fake_time_context_t fake;
    fake_network_context_t fake_network;
    pet_network_provider_t network_provider;
    pet_time_snapshot_t snapshot;
    char text[6];

    /* A: invalid time formats as "--:--" and stays invalid. */
    fake.snapshot = (pet_time_snapshot_t){ .valid = false };
    service = (pet_time_service_t){ &fake, &fake_ops };
    assert(pet_time_service_get_snapshot(&service, &snapshot) == PET_STATUS_OK);
    assert(!snapshot.valid);
    pet_clock_format_unknown(text);
    assert(strcmp(text, "--:--") == 0);

    /* B: 12:34 formats correctly and passes through the service. */
    fake.snapshot = (pet_time_snapshot_t){ .valid = true, .hour = 12, .minute = 34,
                                           .second = 7 };
    assert(pet_time_service_get_snapshot(&service, &snapshot) == PET_STATUS_OK);
    assert(snapshot.valid && snapshot.hour == 12U && snapshot.minute == 34U &&
           snapshot.second == 7U);
    pet_clock_format_hhmm(12, 34, text);
    assert(strcmp(text, "12:34") == 0);

    /* C: minute rollover formatting. */
    pet_clock_format_hhmm(23, 59, text);
    assert(strcmp(text, "23:59") == 0);
    pet_clock_format_hhmm(0, 0, text);
    assert(strcmp(text, "00:00") == 0);

    /* System backend smoke test: valid on the host with in-range fields. */
    assert(pet_time_service_init_system(&service) == PET_STATUS_OK);
    assert(pet_time_service_get_snapshot(&service, &snapshot) == PET_STATUS_OK);
    assert(snapshot.valid);
    assert(snapshot.hour < 24U);
    assert(snapshot.minute < 60U);
    assert(snapshot.second < 61U);

    /* Error paths. */
    assert(pet_time_service_get_snapshot(NULL, &snapshot) ==
           PET_STATUS_INVALID_ARGUMENT);
    service = (pet_time_service_t){ NULL, NULL };
    assert(pet_time_service_get_snapshot(&service, &snapshot) ==
           PET_STATUS_INVALID_ARGUMENT);

    /* Network state names. */
    fake_network = (fake_network_context_t){ PET_NETWORK_CONNECTING };
    network_provider = (pet_network_provider_t){ &fake_network,
                                                 fake_get_network_state };
    assert(pet_network_provider_get_state(&network_provider) ==
           PET_NETWORK_CONNECTING);
    fake_network.state = PET_NETWORK_CONNECTED;
    assert(pet_network_provider_get_state(&network_provider) ==
           PET_NETWORK_CONNECTED);
    assert(pet_network_provider_get_state(NULL) == PET_NETWORK_DISCONNECTED);
    assert(strcmp(pet_network_state_name(PET_NETWORK_DISCONNECTED), "disconnected") == 0);
    assert(strcmp(pet_network_state_name(PET_NETWORK_CONNECTING), "connecting") == 0);
    assert(strcmp(pet_network_state_name(PET_NETWORK_CONNECTED), "connected") == 0);
    assert(strcmp(pet_network_state_name(PET_NETWORK_ERROR), "error") == 0);

    (void)printf("test_time_service: ok\n");
    return 0;
}

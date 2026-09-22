#ifndef PET_NETWORK_H
#define PET_NETWORK_H

typedef enum {
    PET_NETWORK_DISCONNECTED = 0,
    PET_NETWORK_CONNECTING,
    PET_NETWORK_CONNECTED,
    PET_NETWORK_ERROR
} pet_network_state_t;

typedef struct {
    void *context;
    pet_network_state_t (*get_state)(void *context);
} pet_network_provider_t;

pet_network_state_t pet_network_provider_get_state(
    const pet_network_provider_t *provider);
const char *pet_network_state_name(pet_network_state_t state);

#endif

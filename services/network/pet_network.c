#include "services/pet_network.h"

#include <stddef.h>

pet_network_state_t pet_network_provider_get_state(
    const pet_network_provider_t *provider)
{
    if (provider == NULL || provider->get_state == NULL) {
        return PET_NETWORK_DISCONNECTED;
    }
    return provider->get_state(provider->context);
}

const char *pet_network_state_name(pet_network_state_t state)
{
    switch (state) {
    case PET_NETWORK_CONNECTING:
        return "connecting";
    case PET_NETWORK_CONNECTED:
        return "connected";
    case PET_NETWORK_ERROR:
        return "error";
    case PET_NETWORK_DISCONNECTED:
    default:
        return "disconnected";
    }
}

#ifndef PET_ESP32_WIFI_H
#define PET_ESP32_WIFI_H

#include "pet/status.h"
#include "services/pet_network.h"

/* Starts the Wi-Fi STA backend in the background (ESP-IDF event task model).
 * Never blocks. Without local credentials (wifi_config.local.h) the provider
 * stays in PET_NETWORK_DISCONNECTED and the pet keeps running. */
pet_status_t pet_esp32_wifi_start(pet_network_provider_t *provider);

#endif

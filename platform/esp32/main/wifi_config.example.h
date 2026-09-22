/*
 * Template for local Wi-Fi credentials.
 *
 * Copy this file to `wifi_config.local.h` in the same directory, fill in your
 * real SSID and password, and never commit that file (it is listed in
 * .gitignore). When the local header is missing the firmware boots with Wi-Fi
 * disabled and logs a warning; the pet keeps running normally.
 */
#ifndef PET_WIFI_CONFIG_EXAMPLE_H
#define PET_WIFI_CONFIG_EXAMPLE_H

#define PET_WIFI_SSID     "your-wifi-ssid"
#define PET_WIFI_PASSWORD "your-wifi-password"

#endif

#include "pet_esp32_wifi.h"

#include <stdbool.h>
#include <string.h>
#include <time.h>

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_netif_sntp.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "nvs_flash.h"

#include "services/pet_clock_format.h"
#include "services/pet_time_service.h"

#ifdef PET_WIFI_CONFIG_LOCAL
#include "wifi_config.local.h"
#endif

#define WIFI_TAG             "pet_wifi"
#define WIFI_RECONNECT_MIN_S 1U
#define WIFI_RECONNECT_MAX_S 30U

static pet_network_state_t s_state = PET_NETWORK_DISCONNECTED;
static esp_timer_handle_t s_reconnect_timer;
static uint32_t s_reconnect_backoff_s = WIFI_RECONNECT_MIN_S;
static bool s_sntp_started;

static pet_network_state_t get_state(void *context)
{
    (void)context;
    return s_state;
}

static void suppress_verbose_wifi_logs(void)
{
    esp_log_level_set("wifi", ESP_LOG_WARN);
    esp_log_level_set("wifi_init", ESP_LOG_WARN);
    esp_log_level_set("net80211", ESP_LOG_WARN);
    esp_log_level_set("pp", ESP_LOG_WARN);
}

static void log_synced_time(void)
{
    time_t now = time(NULL);
    struct tm time_info;
    char text[6];
    if (now >= (time_t)PET_TIME_VALID_MIN_EPOCH &&
        localtime_r(&now, &time_info) != NULL) {
        pet_clock_format_hhmm((uint8_t)time_info.tm_hour,
                              (uint8_t)time_info.tm_min, text);
        ESP_LOGI(WIFI_TAG, "Time synchronized: %s", text);
    } else {
        ESP_LOGI(WIFI_TAG, "Time synchronized");
    }
}

static void sntp_sync_cb(struct timeval *tv)
{
    (void)tv;
    log_synced_time();
}

static void start_sntp(void)
{
    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
    if (s_sntp_started) {
        return;
    }
    s_sntp_started = true;
    config.sync_cb = sntp_sync_cb;
    ESP_LOGI(WIFI_TAG, "SNTP sync started");
    if (esp_netif_sntp_init(&config) != ESP_OK) {
        ESP_LOGW(WIFI_TAG, "SNTP init failed");
        s_sntp_started = false;
    }
}

static void schedule_reconnect(void)
{
    if (s_reconnect_timer == NULL) {
        return;
    }
    esp_timer_stop(s_reconnect_timer);
    ESP_LOGI(WIFI_TAG, "WiFi reconnect scheduled in %u s",
             (unsigned)s_reconnect_backoff_s);
    esp_timer_start_once(s_reconnect_timer,
                         (uint64_t)s_reconnect_backoff_s * 1000000ULL);
    if (s_reconnect_backoff_s < WIFI_RECONNECT_MAX_S) {
        s_reconnect_backoff_s *= 2U;
        if (s_reconnect_backoff_s > WIFI_RECONNECT_MAX_S) {
            s_reconnect_backoff_s = WIFI_RECONNECT_MAX_S;
        }
    }
}

static void reconnect_timer_cb(void *arg)
{
    (void)arg;
    if (s_state == PET_NETWORK_CONNECTED) {
        return;
    }
    s_state = PET_NETWORK_CONNECTING;
    esp_wifi_connect();
}

static void wifi_event_handler(void *arg, esp_event_base_t base, int32_t id,
                               void *data)
{
    (void)arg;
    (void)data;
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        esp_err_t result;
        s_state = PET_NETWORK_CONNECTING;
        ESP_LOGI(WIFI_TAG, "WiFi connecting...");
        result = esp_wifi_connect();
        if (result != ESP_OK) {
            s_state = PET_NETWORK_ERROR;
            ESP_LOGW(WIFI_TAG, "WiFi connect failed: %s", esp_err_to_name(result));
            schedule_reconnect();
        }
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_CONNECTED) {
        ESP_LOGI(WIFI_TAG, "WiFi connected");
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        s_state = PET_NETWORK_CONNECTED;
        s_reconnect_backoff_s = WIFI_RECONNECT_MIN_S;
        ESP_LOGI(WIFI_TAG, "IP acquired");
        start_sntp();
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        s_state = PET_NETWORK_DISCONNECTED;
        ESP_LOGI(WIFI_TAG, "WiFi disconnected");
        schedule_reconnect();
    }
}

pet_status_t pet_esp32_wifi_start(pet_network_provider_t *provider)
{
    esp_err_t result;

    if (provider == NULL) {
        return PET_STATUS_INVALID_ARGUMENT;
    }
    provider->context = NULL;
    provider->get_state = get_state;
    suppress_verbose_wifi_logs();

    result = esp_netif_init();
    if (result != ESP_OK && result != ESP_ERR_INVALID_STATE) {
        return PET_STATUS_IO_ERROR;
    }
    result = esp_event_loop_create_default();
    if (result != ESP_OK && result != ESP_ERR_INVALID_STATE) {
        return PET_STATUS_IO_ERROR;
    }
    if (esp_netif_create_default_wifi_sta() == NULL) {
        return PET_STATUS_NO_MEMORY;
    }
    if (esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                   &wifi_event_handler, NULL) != ESP_OK ||
        esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                   &wifi_event_handler, NULL) != ESP_OK) {
        return PET_STATUS_IO_ERROR;
    }
    if (esp_timer_create(&(esp_timer_create_args_t){ .callback = reconnect_timer_cb,
                                                    .name = "wifi_reconnect" },
                         &s_reconnect_timer) != ESP_OK) {
        return PET_STATUS_NO_MEMORY;
    }

#ifdef PET_WIFI_CONFIG_LOCAL
    {
        wifi_init_config_t init_config = WIFI_INIT_CONFIG_DEFAULT();
        wifi_config_t wifi_config = {
            .sta = {
                .ssid = PET_WIFI_SSID,
                .password = PET_WIFI_PASSWORD,
                .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            },
        };
        ESP_LOGI(WIFI_TAG, "Wi-Fi init: local credentials loaded");
        if (nvs_flash_init() != ESP_OK ||
            esp_wifi_init(&init_config) != ESP_OK ||
            esp_wifi_set_storage(WIFI_STORAGE_RAM) != ESP_OK ||
            esp_wifi_set_mode(WIFI_MODE_STA) != ESP_OK ||
            esp_wifi_set_config(WIFI_IF_STA, &wifi_config) != ESP_OK ||
            esp_wifi_start() != ESP_OK) {
            s_state = PET_NETWORK_ERROR;
            ESP_LOGE(WIFI_TAG, "Wi-Fi init failed");
            return PET_STATUS_IO_ERROR;
        }
    }
#else
    ESP_LOGW(WIFI_TAG,
             "Wi-Fi disabled: no local credentials (wifi_config.local.h)");
#endif
    return PET_STATUS_OK;
}

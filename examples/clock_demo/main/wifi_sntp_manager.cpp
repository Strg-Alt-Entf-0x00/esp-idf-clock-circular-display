/**
 * @file wifi_sntp_manager.cpp
 * @brief Wi-Fi connectivity and SNTP synchronization helper for the demo
 */

#include "wifi_sntp_manager.h"
#include <atomic>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "esp_log.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_sntp.h"
#include "nvs_flash.h"
#include "soc/soc_caps.h"

#if defined(SOC_WIFI_SUPPORTED) && SOC_WIFI_SUPPORTED
#include "esp_wifi.h"
static esp_event_handler_instance_t s_wifi_event_instance;
static esp_event_handler_instance_t s_ip_event_instance;
#endif

static const char* TAG = "wifi_sntp";

static std::atomic<bool> s_wifi_connected{false};
static std::atomic<bool> s_time_synced{false};
static wifi_sntp_sync_cb_t s_on_sync_cb = NULL;

[[maybe_unused]] static void time_sync_notification_cb(struct timeval* tv) {
    if (!tv) {
        ESP_LOGE(TAG, "SNTP notification did not include a timestamp");
        return;
    }
    time_t now = tv->tv_sec;
    struct tm timeinfo;
    if (!localtime_r(&now, &timeinfo)) {
        ESP_LOGE(TAG, "Failed to convert synchronized time to local time");
        return;
    }
    s_time_synced.store(true);
    ESP_LOGI(TAG, "SNTP synchronized local system time: %04d-%02d-%02d %02d:%02d:%02d",
             timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
             timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

    if (s_on_sync_cb) {
        s_on_sync_cb();
    }
}

#if defined(SOC_WIFI_SUPPORTED) && SOC_WIFI_SUPPORTED
static void event_handler(void* arg, esp_event_base_t event_base,
                          int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "WiFi station started -> connecting to AP...");
        esp_err_t ret = esp_wifi_connect();
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Initial Wi-Fi connection attempt failed: %s",
                     esp_err_to_name(ret));
        }
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        s_wifi_connected.store(false);
        if (esp_sntp_enabled()) {
            esp_sntp_stop();
        }
        ESP_LOGW(TAG, "WiFi disconnected, attempting reconnection...");
        esp_err_t ret = esp_wifi_connect();
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Wi-Fi reconnection attempt failed: %s",
                     esp_err_to_name(ret));
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        if (!event_data) {
            ESP_LOGE(TAG, "Got-IP event did not include event data");
            return;
        }
        const ip_event_got_ip_t* event = static_cast<const ip_event_got_ip_t*>(event_data);
        ESP_LOGI(TAG, "WiFi connected! Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        s_wifi_connected.store(true);

        if (!esp_sntp_enabled()) {
            ESP_LOGI(TAG, "Starting SNTP synchronization");
            esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
            esp_sntp_setservername(0, "pool.ntp.org");
            esp_sntp_setservername(1, "time.google.com");
            esp_sntp_setservername(2, "ptbtime1.ptb.de"); // Physikalisch-Technische Bundesanstalt Atomuhr
            esp_sntp_set_time_sync_notification_cb(time_sync_notification_cb);
            esp_sntp_init();
        }
    }
}
#endif

esp_err_t wifi_sntp_manager_start(const char* ssid, const char* password, wifi_sntp_sync_cb_t on_sync_cb) {
    s_on_sync_cb = on_sync_cb;

#if defined(SOC_WIFI_SUPPORTED) && SOC_WIFI_SUPPORTED
    wifi_config_t wifi_config = {};
    if (!ssid || ssid[0] == '\0' || strcmp(ssid, "YOUR_WIFI_SSID") == 0 ||
        strlen(ssid) >= sizeof(wifi_config.sta.ssid) ||
        (password && strlen(password) >= sizeof(wifi_config.sta.password))) {
        ESP_LOGE(TAG, "Wi-Fi credentials are missing or exceed the supported size");
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret = nvs_flash_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "NVS initialization failed: %s; refusing to erase NVS automatically",
                 esp_err_to_name(ret));
        return ret;
    }

    ret = esp_netif_init();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "esp_netif_init failed: %s", esp_err_to_name(ret));
        return ret;
    }
    ret = esp_event_loop_create_default();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "Default event loop creation failed: %s", esp_err_to_name(ret));
        return ret;
    }

    if (!esp_netif_create_default_wifi_sta()) {
        ESP_LOGE(TAG, "Failed to create the default Wi-Fi station interface");
        return ESP_FAIL;
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ret = esp_wifi_init(&cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                              &event_handler, NULL,
                                              &s_wifi_event_instance);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Wi-Fi event registration failed: %s", esp_err_to_name(ret));
        return ret;
    }
    ret = esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                              &event_handler, NULL,
                                              &s_ip_event_instance);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "IP event registration failed: %s", esp_err_to_name(ret));
        return ret;
    }

    strncpy((char*)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid) - 1);
    if (password) {
        strncpy((char*)wifi_config.sta.password, password, sizeof(wifi_config.sta.password) - 1);
    }
    wifi_config.sta.threshold.authmode = (password && password[0] != '\0') ? WIFI_AUTH_WPA2_PSK : WIFI_AUTH_OPEN;

    ret = esp_wifi_set_mode(WIFI_MODE_STA);
    if (ret == ESP_OK) ret = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    if (ret == ESP_OK) ret = esp_wifi_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start Wi-Fi station: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "Wi-Fi station configured");
    return ESP_OK;
#else
    (void)ssid;
    (void)password;
    ESP_LOGE(TAG, "This target has no built-in Wi-Fi SNTP backend");
    return ESP_ERR_NOT_SUPPORTED;
#endif
}

bool wifi_sntp_manager_is_connected(void) {
    return s_wifi_connected.load();
}

bool wifi_sntp_manager_is_time_synced(void) {
    return s_time_synced.load();
}

/**
 * @file wifi_sntp_manager.h
 * @brief WiFi connection and SNTP atomic time synchronization manager
 */

#pragma once

#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*wifi_sntp_sync_cb_t)(void);

/**
 * @brief Initialize WiFi and SNTP synchronization
 * @param ssid WiFi SSID
 * @param password WiFi Password
 * @param on_sync_cb Callback invoked when atomic time is acquired from NTP
 * @return ESP_OK if started, or error code
 */
esp_err_t wifi_sntp_manager_start(const char* ssid, const char* password, wifi_sntp_sync_cb_t on_sync_cb);

/**
 * @brief Check if WiFi is currently connected
 */
bool wifi_sntp_manager_is_connected(void);

/**
 * @brief Check if atomic NTP time has been acquired
 */
bool wifi_sntp_manager_is_time_synced(void);

#ifdef __cplusplus
}
#endif

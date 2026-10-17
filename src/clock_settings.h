/**
 * @file clock_settings.h
 * @brief Persistent settings manager for circular clock display
 * 
 * Manages runtime configuration with NVS persistence:
 * - Locale (date language and formatting)
 * - Timezone (POSIX TZ string)
 * - Date format (numeric, abbreviated, full weekday)
 * - Badge mode (smart auto, always antenna, always mode)
 * - 12h/24h mode
 * - Display brightness
 */

#pragma once

#include "esp_err.h"
#include "circular_clock.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CLOCK_SETTINGS_LOCALE_MAX_LEN   15      /**< Max locale tag length (e.g. "en-US") */
#define CLOCK_SETTINGS_TIMEZONE_MAX_LEN 63      /**< Max POSIX TZ string length */

/**
 * @brief Complete clock settings structure
 */
typedef struct {
    char locale[CLOCK_SETTINGS_LOCALE_MAX_LEN + 1];         /**< BCP 47 locale tag */
    char timezone[CLOCK_SETTINGS_TIMEZONE_MAX_LEN + 1];     /**< POSIX timezone string */
    clock_date_format_t date_format;                        /**< Date display format */
    clock_badge_mode_t badge_mode;                          /**< Badge display mode */
    bool ampm_mode;                                         /**< 12h AM/PM mode (false=24h) */
    uint8_t brightness;                                     /**< Display brightness 0-100% */
} clock_settings_t;

/**
 * @brief Initialize settings manager and load from NVS
 * 
 * If NVS is empty or corrupt, uses and saves default settings.
 * Call once during clock initialization.
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t clock_settings_init(void);

/**
 * @brief Load current settings from NVS
 * @param settings Output structure to fill
 * @return ESP_OK on success, ESP_ERR_NVS_NOT_FOUND if no settings exist
 */
esp_err_t clock_settings_load(clock_settings_t* settings);

/**
 * @brief Save current settings to NVS
 * @param settings Settings to save
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t clock_settings_save(const clock_settings_t* settings);

/**
 * @brief Reset to default settings and save to NVS
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t clock_settings_reset_to_defaults(void);

/**
 * @brief Set locale with validation
 * @param locale BCP 47 locale tag (e.g. "de-DE", "en-US")
 * @return ESP_OK, ESP_ERR_INVALID_ARG, or ESP_ERR_NOT_SUPPORTED
 */
esp_err_t clock_settings_set_locale(const char* locale);

/**
 * @brief Set timezone with validation
 * @param timezone POSIX timezone string (e.g. "CET-1CEST,M3.5.0,M10.5.0/3")
 * @return ESP_OK, ESP_ERR_INVALID_ARG, or ESP_ERR_INVALID_SIZE
 */
esp_err_t clock_settings_set_timezone(const char* timezone);

/**
 * @brief Set date format with validation
 * @param format Date format enum value
 * @return ESP_OK or ESP_ERR_INVALID_ARG
 */
esp_err_t clock_settings_set_date_format(clock_date_format_t format);

/**
 * @brief Set badge mode with validation
 * @param mode Badge mode enum value
 * @return ESP_OK or ESP_ERR_INVALID_ARG
 */
esp_err_t clock_settings_set_badge_mode(clock_badge_mode_t mode);

/**
 * @brief Set 12h/24h mode
 * @param enable_ampm true for 12h AM/PM, false for 24h
 * @return ESP_OK
 */
esp_err_t clock_settings_set_ampm_mode(bool enable_ampm);

/**
 * @brief Set display brightness with validation
 * @param brightness Brightness 0-100%
 * @return ESP_OK or ESP_ERR_INVALID_ARG
 */
esp_err_t clock_settings_set_brightness(uint8_t brightness);

/**
 * @brief Get current locale
 * @param buf Output buffer (min 16 bytes)
 * @param size Buffer size
 * @return ESP_OK or ESP_ERR_INVALID_SIZE
 */
esp_err_t clock_settings_get_locale(char* buf, size_t size);

/**
 * @brief Get current timezone
 * @param buf Output buffer (min 64 bytes)
 * @param size Buffer size
 * @return ESP_OK or ESP_ERR_INVALID_SIZE
 */
esp_err_t clock_settings_get_timezone(char* buf, size_t size);

/**
 * @brief Get current date format
 */
clock_date_format_t clock_settings_get_date_format(void);

/**
 * @brief Get current badge mode
 */
clock_badge_mode_t clock_settings_get_badge_mode(void);

/**
 * @brief Get current 12h/24h mode
 * @return true if 12h AM/PM, false if 24h
 */
bool clock_settings_get_ampm_mode(void);

/**
 * @brief Get current brightness
 * @return Brightness 0-100%
 */
uint8_t clock_settings_get_brightness(void);

/**
 * @brief Apply current settings to clock display
 * 
 * Updates locale, timezone, date format, badge mode, and brightness.
 * Call after loading settings or changing multiple settings.
 * 
 * @return ESP_OK on success
 */
esp_err_t clock_settings_apply_to_clock(void);

#ifdef __cplusplus
}
#endif

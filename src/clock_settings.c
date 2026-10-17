/**
 * @file clock_settings.c
 * @brief Implementation of persistent settings manager
 */

#include "clock_settings.h"
#include "locales/locale_data.h"
#include "circular_clock.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include <string.h>
#include <time.h>

static const char* TAG = "clock_settings";

#define NVS_NAMESPACE "clock_cfg"
#define NVS_KEY_LOCALE      "locale"
#define NVS_KEY_TIMEZONE    "timezone"
#define NVS_KEY_DATE_FMT    "date_fmt"
#define NVS_KEY_BADGE_MODE  "badge_mode"
#define NVS_KEY_AMPM_MODE   "ampm_mode"
#define NVS_KEY_BRIGHTNESS  "brightness"

// Default settings
static const clock_settings_t DEFAULT_SETTINGS = {
    .locale = "de-DE",
    .timezone = "CET-1CEST,M3.5.0,M10.5.0/3",
    .date_format = CLOCK_DATE_FMT_ABBREV,
    .badge_mode = CLOCK_BADGE_SMART_AUTO,
    .ampm_mode = false,
    .brightness = 100
};

// Current runtime settings
static clock_settings_t s_current_settings;
static bool s_initialized = false;

// ============================================================================
// Validation Helpers
// ============================================================================

static bool is_valid_timezone(const char* tz) {
    if (!tz || !tz[0]) {
        return false;
    }
    
    size_t length = strlen(tz);
    if (length > CLOCK_SETTINGS_TIMEZONE_MAX_LEN) {
        return false;
    }
    
    // Basic check: must contain only printable ASCII
    for (size_t i = 0; i < length; ++i) {
        if (tz[i] < 0x20 || tz[i] > 0x7E) {
            return false;
        }
    }
    
    return true;
}

static bool is_valid_date_format(clock_date_format_t fmt) {
    return fmt >= CLOCK_DATE_FMT_NUMERIC && fmt <= CLOCK_DATE_FMT_FULL_WEEKDAY;
}

static bool is_valid_badge_mode(clock_badge_mode_t mode) {
    return mode >= CLOCK_BADGE_SMART_AUTO && mode <= CLOCK_BADGE_ALWAYS_MODE;
}

static bool is_valid_brightness(uint8_t brightness) {
    return brightness <= 100;
}

// ============================================================================
// NVS Operations
// ============================================================================

static esp_err_t load_from_nvs(clock_settings_t* settings) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK) {
        return err;
    }
    
    // Load locale
    size_t locale_len = sizeof(settings->locale);
    err = nvs_get_str(handle, NVS_KEY_LOCALE, settings->locale, &locale_len);
    if (err != ESP_OK) {
        nvs_close(handle);
        return err;
    }
    
    // Load timezone
    size_t tz_len = sizeof(settings->timezone);
    err = nvs_get_str(handle, NVS_KEY_TIMEZONE, settings->timezone, &tz_len);
    if (err != ESP_OK) {
        nvs_close(handle);
        return err;
    }
    
    // Load date format
    uint8_t date_fmt;
    err = nvs_get_u8(handle, NVS_KEY_DATE_FMT, &date_fmt);
    if (err != ESP_OK) {
        nvs_close(handle);
        return err;
    }
    settings->date_format = (clock_date_format_t)date_fmt;
    
    // Load badge mode
    uint8_t badge_mode;
    err = nvs_get_u8(handle, NVS_KEY_BADGE_MODE, &badge_mode);
    if (err != ESP_OK) {
        nvs_close(handle);
        return err;
    }
    settings->badge_mode = (clock_badge_mode_t)badge_mode;
    
    // Load AM/PM mode
    uint8_t ampm_mode;
    err = nvs_get_u8(handle, NVS_KEY_AMPM_MODE, &ampm_mode);
    if (err != ESP_OK) {
        nvs_close(handle);
        return err;
    }
    settings->ampm_mode = (ampm_mode != 0);
    
    // Load brightness
    err = nvs_get_u8(handle, NVS_KEY_BRIGHTNESS, &settings->brightness);
    if (err != ESP_OK) {
        nvs_close(handle);
        return err;
    }
    
    nvs_close(handle);
    return ESP_OK;
}

static esp_err_t save_to_nvs(const clock_settings_t* settings) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS handle: %s", esp_err_to_name(err));
        return err;
    }
    
    // Save locale
    err = nvs_set_str(handle, NVS_KEY_LOCALE, settings->locale);
    if (err != ESP_OK) goto cleanup;
    
    // Save timezone
    err = nvs_set_str(handle, NVS_KEY_TIMEZONE, settings->timezone);
    if (err != ESP_OK) goto cleanup;
    
    // Save date format
    err = nvs_set_u8(handle, NVS_KEY_DATE_FMT, (uint8_t)settings->date_format);
    if (err != ESP_OK) goto cleanup;
    
    // Save badge mode
    err = nvs_set_u8(handle, NVS_KEY_BADGE_MODE, (uint8_t)settings->badge_mode);
    if (err != ESP_OK) goto cleanup;
    
    // Save AM/PM mode
    err = nvs_set_u8(handle, NVS_KEY_AMPM_MODE, settings->ampm_mode ? 1 : 0);
    if (err != ESP_OK) goto cleanup;
    
    // Save brightness
    err = nvs_set_u8(handle, NVS_KEY_BRIGHTNESS, settings->brightness);
    if (err != ESP_OK) goto cleanup;
    
    // Commit
    err = nvs_commit(handle);
    
cleanup:
    nvs_close(handle);
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save settings to NVS: %s", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "Settings saved to NVS");
    }
    
    return err;
}

// ============================================================================
// Public API Implementation
// ============================================================================

esp_err_t clock_settings_init(void) {
    if (s_initialized) {
        return ESP_OK;
    }
    
    // Initialize NVS if not already done
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS partition needs erasing, erasing...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize NVS: %s", esp_err_to_name(err));
        return err;
    }
    
    // Try to load settings from NVS
    err = load_from_nvs(&s_current_settings);
    
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        // First boot: use defaults and save
        ESP_LOGI(TAG, "No settings found in NVS, using defaults");
        memcpy(&s_current_settings, &DEFAULT_SETTINGS, sizeof(clock_settings_t));
        err = save_to_nvs(&s_current_settings);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "Could not save default settings: %s", esp_err_to_name(err));
        }
    } else if (err != ESP_OK) {
        // Other error: use defaults but log warning
        ESP_LOGW(TAG, "Failed to load settings (%s), using defaults", esp_err_to_name(err));
        memcpy(&s_current_settings, &DEFAULT_SETTINGS, sizeof(clock_settings_t));
    } else {
        ESP_LOGI(TAG, "Loaded settings from NVS:");
        ESP_LOGI(TAG, "  Locale: %s", s_current_settings.locale);
        ESP_LOGI(TAG, "  Timezone: %s", s_current_settings.timezone);
        ESP_LOGI(TAG, "  Date format: %d", s_current_settings.date_format);
        ESP_LOGI(TAG, "  Badge mode: %d", s_current_settings.badge_mode);
        ESP_LOGI(TAG, "  12h mode: %s", s_current_settings.ampm_mode ? "yes" : "no");
        ESP_LOGI(TAG, "  Brightness: %d%%", s_current_settings.brightness);
    }
    
    s_initialized = true;
    return ESP_OK;
}

esp_err_t clock_settings_load(clock_settings_t* settings) {
    if (!settings) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (!s_initialized) {
        esp_err_t err = clock_settings_init();
        if (err != ESP_OK) {
            return err;
        }
    }
    
    memcpy(settings, &s_current_settings, sizeof(clock_settings_t));
    return ESP_OK;
}

esp_err_t clock_settings_save(const clock_settings_t* settings) {
    if (!s_initialized) {
        esp_err_t err = clock_settings_init();
        if (err != ESP_OK) {
            return err;
        }
    }
    
    if (settings != NULL) {
        // Validate all settings before saving
        if (!locale_data_find(settings->locale)) {
            ESP_LOGE(TAG, "Invalid locale: %s", settings->locale);
            return ESP_ERR_INVALID_ARG;
        }
        
        if (!is_valid_timezone(settings->timezone)) {
            ESP_LOGE(TAG, "Invalid timezone");
            return ESP_ERR_INVALID_ARG;
        }
        
        if (!is_valid_date_format(settings->date_format)) {
            ESP_LOGE(TAG, "Invalid date format: %d", settings->date_format);
            return ESP_ERR_INVALID_ARG;
        }
        
        if (!is_valid_badge_mode(settings->badge_mode)) {
            ESP_LOGE(TAG, "Invalid badge mode: %d", settings->badge_mode);
            return ESP_ERR_INVALID_ARG;
        }
        
        if (!is_valid_brightness(settings->brightness)) {
            ESP_LOGE(TAG, "Invalid brightness: %d", settings->brightness);
            return ESP_ERR_INVALID_ARG;
        }
        
        // Update current settings
        memcpy(&s_current_settings, settings, sizeof(clock_settings_t));
    }
    
    // Save to NVS
    return save_to_nvs(&s_current_settings);
}

esp_err_t clock_settings_reset_to_defaults(void) {
    if (!s_initialized) {
        esp_err_t err = clock_settings_init();
        if (err != ESP_OK) {
            return err;
        }
    }
    
    ESP_LOGI(TAG, "Resetting settings to defaults");
    memcpy(&s_current_settings, &DEFAULT_SETTINGS, sizeof(clock_settings_t));
    esp_err_t err = save_to_nvs(&s_current_settings);
    if (err == ESP_OK) {
        clock_settings_apply_to_clock();
    }
    return err;
}

esp_err_t clock_settings_set_locale(const char* locale) {
    if (!s_initialized) {
        clock_settings_init();
    }
    
    if (!locale || !locale[0]) {
        return ESP_ERR_INVALID_ARG;
    }
    
    const locale_definition_t* loc = locale_data_find(locale);
    if (!loc) {
        ESP_LOGE(TAG, "Unsupported locale: %s", locale);
        return ESP_ERR_NOT_SUPPORTED;
    }
    
    strncpy(s_current_settings.locale, loc->tag, CLOCK_SETTINGS_LOCALE_MAX_LEN);
    s_current_settings.locale[CLOCK_SETTINGS_LOCALE_MAX_LEN] = '\0';
    
    // Apply immediately to active clock display
    circular_clock_set_locale(s_current_settings.locale);
    
    ESP_LOGI(TAG, "Locale set to: %s", s_current_settings.locale);
    return ESP_OK;
}

esp_err_t clock_settings_set_timezone(const char* timezone) {
    if (!s_initialized) {
        clock_settings_init();
    }
    
    if (!timezone || !timezone[0]) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (!is_valid_timezone(timezone)) {
        ESP_LOGE(TAG, "Invalid timezone string");
        return ESP_ERR_INVALID_ARG;
    }
    
    size_t len = strlen(timezone);
    if (len > CLOCK_SETTINGS_TIMEZONE_MAX_LEN) {
        ESP_LOGE(TAG, "Timezone string too long: %zu (max %d)", 
                 len, CLOCK_SETTINGS_TIMEZONE_MAX_LEN);
        return ESP_ERR_INVALID_SIZE;
    }
    
    strcpy(s_current_settings.timezone, timezone);
    
    // Apply immediately to POSIX environment
    setenv("TZ", s_current_settings.timezone, 1);
    tzset();
    
    ESP_LOGI(TAG, "Timezone set to: %s", s_current_settings.timezone);
    return ESP_OK;
}

esp_err_t clock_settings_set_date_format(clock_date_format_t format) {
    if (!s_initialized) {
        clock_settings_init();
    }
    
    if (!is_valid_date_format(format)) {
        ESP_LOGE(TAG, "Invalid date format: %d", format);
        return ESP_ERR_INVALID_ARG;
    }
    
    s_current_settings.date_format = format;
    circular_clock_set_date_format(format);
    
    ESP_LOGI(TAG, "Date format set to: %d", format);
    return ESP_OK;
}

esp_err_t clock_settings_set_badge_mode(clock_badge_mode_t mode) {
    if (!s_initialized) {
        clock_settings_init();
    }
    
    if (!is_valid_badge_mode(mode)) {
        ESP_LOGE(TAG, "Invalid badge mode: %d", mode);
        return ESP_ERR_INVALID_ARG;
    }
    
    s_current_settings.badge_mode = mode;
    circular_clock_set_badge_mode(mode);
    
    ESP_LOGI(TAG, "Badge mode set to: %d", mode);
    return ESP_OK;
}

esp_err_t clock_settings_set_ampm_mode(bool enable_ampm) {
    if (!s_initialized) {
        clock_settings_init();
    }
    
    s_current_settings.ampm_mode = enable_ampm;
    circular_clock_set_ampm_mode(enable_ampm, false);
    
    ESP_LOGI(TAG, "12h mode: %s", enable_ampm ? "enabled" : "disabled");
    return ESP_OK;
}

esp_err_t clock_settings_set_brightness(uint8_t brightness) {
    if (!s_initialized) {
        clock_settings_init();
    }
    
    if (!is_valid_brightness(brightness)) {
        ESP_LOGE(TAG, "Invalid brightness: %d (must be 0-100)", brightness);
        return ESP_ERR_INVALID_ARG;
    }
    
    s_current_settings.brightness = brightness;
    circular_clock_set_brightness(brightness);
    
    ESP_LOGI(TAG, "Brightness set to: %d%%", brightness);
    return ESP_OK;
}

esp_err_t clock_settings_get_locale(char* buf, size_t size) {
    if (!buf || size < 16) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (!s_initialized) {
        clock_settings_init();
    }
    
    strncpy(buf, s_current_settings.locale, size - 1);
    buf[size - 1] = '\0';
    return ESP_OK;
}

esp_err_t clock_settings_get_timezone(char* buf, size_t size) {
    if (!buf || size < 64) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (!s_initialized) {
        clock_settings_init();
    }
    
    strncpy(buf, s_current_settings.timezone, size - 1);
    buf[size - 1] = '\0';
    return ESP_OK;
}

clock_date_format_t clock_settings_get_date_format(void) {
    if (!s_initialized) {
        clock_settings_init();
    }
    return s_current_settings.date_format;
}

clock_badge_mode_t clock_settings_get_badge_mode(void) {
    if (!s_initialized) {
        clock_settings_init();
    }
    return s_current_settings.badge_mode;
}

bool clock_settings_get_ampm_mode(void) {
    if (!s_initialized) {
        clock_settings_init();
    }
    return s_current_settings.ampm_mode;
}

uint8_t clock_settings_get_brightness(void) {
    if (!s_initialized) {
        clock_settings_init();
    }
    return s_current_settings.brightness;
}

esp_err_t clock_settings_apply_to_clock(void) {
    if (!s_initialized) {
        esp_err_t err = clock_settings_init();
        if (err != ESP_OK) {
            return err;
        }
    }
    
    ESP_LOGI(TAG, "Applying settings to clock display");
    
    // Apply locale
    esp_err_t err = circular_clock_set_locale(s_current_settings.locale);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set locale: %s", esp_err_to_name(err));
        return err;
    }
    
    // Apply timezone to system
    setenv("TZ", s_current_settings.timezone, 1);
    tzset();
    ESP_LOGI(TAG, "Timezone applied: %s", s_current_settings.timezone);
    
    // Apply date format
    circular_clock_set_date_format(s_current_settings.date_format);
    
    // Apply badge mode
    circular_clock_set_badge_mode(s_current_settings.badge_mode);
    
    // Apply 12h/24h mode
    circular_clock_set_ampm_mode(s_current_settings.ampm_mode, false);
    
    // Apply brightness
    circular_clock_set_brightness(s_current_settings.brightness);
    
    ESP_LOGI(TAG, "Settings applied successfully");
    return ESP_OK;
}

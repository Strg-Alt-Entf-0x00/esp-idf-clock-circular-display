/**
 * @file circular_clock.cpp
 * @brief Core controller coordinating dial rings, center cluster, and radio animations
 */

#include "circular_clock.h"
#include "circular_clock_theme.h"
#include "dial_renderer.h"
#include "center_renderer.h"
#include "radio_animator.h"
#include "moon_phase.h"
#include "locales/locale_data.h"
#include "clock_settings.h"
#include "esp_timer.h"
#include "esp_log.h"
#include <cstring>

#define LGFX_USE_V1
#include <LovyanGFX.hpp>

static const char* TAG = "circular_clock";

// Declaration from display_hal.cpp
esp_err_t circular_clock_hal_init(const circular_clock_config_t* config);

struct ClockState {
    int hour = 0;
    int minute = 0;
    int second = 0;
    int millisecond = 0;
    bool time_fields_valid = false;
    bool time_source_valid = false;
    bool time_valid = false;

    int year = 0;
    int month = 0;
    int day = 0;
    int wday = 0;
    bool date_valid = false;
    bool moon_phase_valid = false;
    float moon_phase = 0.0f;

    bool custom_date_set = false;
    char date_str[48] = {};

    const locale_definition_t* locale = nullptr;
    clock_date_format_t date_format = CLOCK_DATE_FMT_ABBREV;
    clock_badge_mode_t badge_mode = CLOCK_BADGE_SMART_AUTO;

    circular_radio_status_t radio_status = CIRCULAR_RADIO_UNKNOWN;
    uint32_t lock_acquired_time_ms = 0;

    bool ampm_mode = false;
    bool is_pm = false;
    uint8_t brightness = 100;
};

static ClockState s_clock;

static bool is_valid_date(int year, int month, int day, int wday) {
    if (year < 1 || year > 9999 || month < 1 || month > 12 ||
        wday < 0 || wday > 6) {
        return false;
    }

    static const int days_per_month[] = {31, 28, 31, 30, 31, 30,
                                          31, 31, 30, 31, 30, 31};
    int max_day = days_per_month[month - 1];
    const bool leap_year = (year % 4 == 0) &&
                           ((year % 100 != 0) || (year % 400 == 0));
    if (month == 2 && leap_year) {
        ++max_day;
    }
    if (day < 1 || day > max_day) {
        return false;
    }

    static const int month_offsets[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    int adjusted_year = year;
    if (month < 3) {
        --adjusted_year;
    }
    const int calculated_wday = (adjusted_year + adjusted_year / 4 -
                                 adjusted_year / 100 + adjusted_year / 400 +
                                 month_offsets[month - 1] + day) % 7;
    return calculated_wday == wday;
}

static void update_formatted_date_string(ClockState& state) {
    if (state.custom_date_set || !state.date_valid || !state.locale) return;

    const int wday = state.wday;
    const int month = state.month - 1;
    const locale_definition_t& locale = *state.locale;

    if (state.date_format == CLOCK_DATE_FMT_NUMERIC) {
        if (locale.numeric_month_first) {
            snprintf(state.date_str, sizeof(state.date_str), "%s %02d%c%02d",
                     locale.weekdays_short[wday], state.month,
                     locale.numeric_separator, state.day);
        } else {
            snprintf(state.date_str, sizeof(state.date_str), "%s %02d%c%02d",
                     locale.weekdays_short[wday], state.day,
                     locale.numeric_separator, state.month);
        }
        return;
    }

    const char* weekday = state.date_format == CLOCK_DATE_FMT_FULL_WEEKDAY
                              ? locale.weekdays_full[wday]
                              : locale.weekdays_short[wday];
    if (locale.date_order == LOCALE_DATE_ORDER_MONTH_DAY) {
        snprintf(state.date_str, sizeof(state.date_str), "%s%s%s %d",
                 weekday, locale.weekday_separator, locale.months_short[month],
                 state.day);
    } else {
        snprintf(state.date_str, sizeof(state.date_str), "%s%s%d%s%s",
                 weekday, locale.weekday_separator, state.day,
                 locale.day_month_separator, locale.months_short[month]);
    }
}

esp_err_t circular_clock_init(const circular_clock_config_t* config) {
    ESP_LOGI(TAG, "Initializing 24-hour circular clock display");
    
    // Initialize settings manager and load persisted settings
    esp_err_t err = clock_settings_init();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Settings init failed (%s), using defaults", esp_err_to_name(err));
    }
    
    // Set default locale if not set
    if (!s_clock.locale) {
        s_clock.locale = locale_data_find("de-DE");
    }
    
    update_formatted_date_string(s_clock);
    
    // Initialize hardware
    err = circular_clock_hal_init(config);
    if (err != ESP_OK) {
        return err;
    }
    
    // Apply loaded settings to clock
    err = clock_settings_apply_to_clock();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to apply settings: %s", esp_err_to_name(err));
    }
    
    return ESP_OK;
}

void circular_clock_set_time(int hour, int minute, int second, int millisecond) {
    if (hour < 0 || hour >= 24 || minute < 0 || minute >= 60 ||
        second < 0 || second >= 60 || millisecond < 0 || millisecond >= 1000) {
        ESP_LOGW(TAG, "Rejected invalid time fields (%d:%d:%d.%d)",
                 hour, minute, second, millisecond);
        s_clock.time_fields_valid = false;
        s_clock.time_valid = false;
        if (s_clock.radio_status == CIRCULAR_RADIO_LOCKED) {
            s_clock.radio_status = CIRCULAR_RADIO_UNKNOWN;
        }
        return;
    }

    s_clock.time_fields_valid = true;
    s_clock.hour = hour;
    s_clock.minute = minute;
    s_clock.second = second;
    s_clock.millisecond = millisecond;
    s_clock.is_pm = (s_clock.hour >= 12);
    s_clock.time_valid = s_clock.time_source_valid;
}

void circular_clock_set_time_valid(bool valid) {
    if (valid && !s_clock.time_fields_valid) {
        ESP_LOGW(TAG, "Cannot mark time valid before valid time fields are supplied");
        s_clock.time_source_valid = false;
        s_clock.time_valid = false;
        if (s_clock.radio_status == CIRCULAR_RADIO_LOCKED) {
            s_clock.radio_status = CIRCULAR_RADIO_UNKNOWN;
        }
        return;
    }
    s_clock.time_source_valid = valid;
    s_clock.time_valid = valid && s_clock.time_fields_valid;
    if (!s_clock.time_valid && s_clock.radio_status == CIRCULAR_RADIO_LOCKED) {
        s_clock.radio_status = CIRCULAR_RADIO_UNKNOWN;
    }
}

void circular_clock_set_date(int year, int month, int day, int wday) {
    s_clock.custom_date_set = false;
    s_clock.date_valid = is_valid_date(year, month, day, wday);
    if (s_clock.date_valid) {
        s_clock.year = year;
        s_clock.month = month;
        s_clock.day = day;
        s_clock.wday = wday;
        s_clock.moon_phase_valid = clock_moon_phase_for_date(
            year, month, day, &s_clock.moon_phase);
    } else {
        ESP_LOGW(TAG, "Rejected invalid date fields (%04d-%02d-%02d, weekday %d)",
                 year, month, day, wday);
        s_clock.year = 0;
        s_clock.month = 0;
        s_clock.day = 0;
        s_clock.wday = 0;
        s_clock.moon_phase_valid = false;
        s_clock.moon_phase = 0.0f;
        s_clock.date_str[0] = '\0';
    }
    update_formatted_date_string(s_clock);
}

void circular_clock_set_date_str(const char* date_str) {
    if (date_str && date_str[0] != '\0') {
        size_t length = 0;
        while (length < sizeof(s_clock.date_str) && date_str[length] != '\0') {
            ++length;
        }
        if (length == sizeof(s_clock.date_str)) {
            ESP_LOGW(TAG, "Custom date string exceeds the %u-byte limit; clearing date",
                     (unsigned)(sizeof(s_clock.date_str) - 1));
            s_clock.custom_date_set = false;
            s_clock.date_valid = false;
            s_clock.moon_phase_valid = false;
            s_clock.moon_phase = 0.0f;
            s_clock.date_str[0] = '\0';
            return;
        }
        s_clock.custom_date_set = true;
        s_clock.date_valid = true;
        s_clock.moon_phase_valid = false;
        s_clock.moon_phase = 0.0f;
        memcpy(s_clock.date_str, date_str, length + 1);
    } else {
        s_clock.custom_date_set = false;
        s_clock.date_valid = false;
        s_clock.moon_phase_valid = false;
        s_clock.moon_phase = 0.0f;
        s_clock.date_str[0] = '\0';
        update_formatted_date_string(s_clock);
    }
}

esp_err_t circular_clock_set_locale(const char* locale_tag) {
    if (!locale_tag || !locale_tag[0]) {
        ESP_LOGE(TAG, "Locale tag must not be NULL or empty");
        return ESP_ERR_INVALID_ARG;
    }

    const locale_definition_t* locale = locale_data_find(locale_tag);
    if (!locale) {
        ESP_LOGE(TAG, "Unsupported locale tag: %s", locale_tag);
        return ESP_ERR_NOT_SUPPORTED;
    }

    s_clock.locale = locale;
    update_formatted_date_string(s_clock);
    ESP_LOGI(TAG, "Locale set to: %s", locale->tag);
    return ESP_OK;
}

void circular_clock_set_date_format(clock_date_format_t fmt) {
    if (fmt < CLOCK_DATE_FMT_NUMERIC || fmt > CLOCK_DATE_FMT_FULL_WEEKDAY) {
        ESP_LOGW(TAG, "Rejected invalid date format value %d", static_cast<int>(fmt));
        return;
    }
    s_clock.date_format = fmt;
    update_formatted_date_string(s_clock);
}

void circular_clock_set_badge_mode(clock_badge_mode_t mode) {
    if (mode < CLOCK_BADGE_SMART_AUTO || mode > CLOCK_BADGE_ALWAYS_MODE) {
        ESP_LOGW(TAG, "Rejected invalid badge mode value %d", static_cast<int>(mode));
        return;
    }
    s_clock.badge_mode = mode;
}

void circular_clock_set_radio_status(circular_radio_status_t status) {
    if (status < CIRCULAR_RADIO_FREE_RUN || status > CIRCULAR_RADIO_UNKNOWN) {
        ESP_LOGW(TAG, "Rejected invalid source status value %d; using UNKNOWN",
                 static_cast<int>(status));
        status = CIRCULAR_RADIO_UNKNOWN;
    }
    if (status == CIRCULAR_RADIO_LOCKED && !s_clock.time_valid) {
        status = CIRCULAR_RADIO_UNKNOWN;
    }
    if (s_clock.radio_status != CIRCULAR_RADIO_LOCKED &&
        status == CIRCULAR_RADIO_LOCKED && s_clock.time_valid) {
        // Record timestamp for Smart Auto badge hold
        s_clock.lock_acquired_time_ms = (uint32_t)(esp_timer_get_time() / 1000);
        // Transition to locked: automatically trigger majestic calibration sweep
        circular_clock_trigger_calibration_sweep();
    }
    s_clock.radio_status = status;
}

circular_radio_status_t circular_clock_get_radio_status(void) {
    return s_clock.radio_status;
}

void circular_clock_trigger_calibration_sweep(void) {
    if (!s_clock.time_valid) {
        return;
    }
    float h_prog = (s_clock.hour + s_clock.minute / 60.0f) * 15.0f;
    float m_prog = (s_clock.minute + s_clock.second / 60.0f) * 6.0f;
    g_radio_animator.triggerCalibrationSweep(h_prog, m_prog);
    ESP_LOGI(TAG, "Triggered synchronization calibration sweep");
}

bool circular_clock_is_calibrating(void) {
    return g_radio_animator.isCalibrating();
}

void circular_clock_set_ampm_mode(bool enable_ampm, bool is_pm) {
    s_clock.ampm_mode = enable_ampm;
    s_clock.is_pm = is_pm;
}

void circular_clock_render_to_sprite(void* sprite_ptr) {
    lgfx::LGFX_Sprite* sprite = static_cast<lgfx::LGFX_Sprite*>(sprite_ptr);
    if (!sprite) return;

    // 1. Clear display to deep obsidian black
    sprite->fillSprite(clock_theme::COLOR_BG);

    // 2. Update time-based animation systems
    uint32_t now_ms = (uint32_t)(esp_timer_get_time() / 1000);
    g_radio_animator.update(now_ms);

    // 3. Render dial rings (comets, scales, Trennschiene, radar sweep)
    dial_renderer_draw(sprite, s_clock.hour, s_clock.minute, s_clock.second,
                       s_clock.millisecond, s_clock.time_valid,
                       s_clock.radio_status);

    // 4. Calculate lock recent state (10s duration for Smart Auto)
    bool lock_recent = (s_clock.radio_status == CIRCULAR_RADIO_LOCKED) &&
                       ((now_ms - s_clock.lock_acquired_time_ms) < 10000);

    // 5. Render compact watchmaker center cluster
    center_renderer_draw(sprite, s_clock.hour, s_clock.minute, s_clock.second,
    s_clock.date_str, s_clock.time_valid,
                         s_clock.date_valid, s_clock.moon_phase_valid,
                         s_clock.moon_phase,
                         s_clock.radio_status,
                         s_clock.badge_mode,
                         lock_recent,
                         s_clock.ampm_mode, s_clock.is_pm);
}

void circular_clock_render(void) {
    void* sprite = circular_clock_get_sprite();
    if (!sprite) return;
    circular_clock_render_to_sprite(sprite);
    // 6. Transfer sprite to physical GC9A01 panel
    circular_clock_push_frame();
}

void circular_clock_set_brightness(uint8_t brightness_pct) {
    if (brightness_pct > 100) {
        ESP_LOGW(TAG, "Brightness %d%% out of range, clamping to 100%%", brightness_pct);
        brightness_pct = 100;
    }
    s_clock.brightness = brightness_pct;
    // TODO: Apply to backlight PWM if managed
}

uint8_t circular_clock_get_brightness(void) {
    return s_clock.brightness;
}

esp_err_t circular_clock_get_locale(char* buf, size_t size) {
    if (!buf || size < 16) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (!s_clock.locale) {
        buf[0] = '\0';
        return ESP_ERR_INVALID_STATE;
    }
    
    strncpy(buf, s_clock.locale->tag, size - 1);
    buf[size - 1] = '\0';
    return ESP_OK;
}

clock_date_format_t circular_clock_get_date_format(void) {
    return s_clock.date_format;
}

clock_badge_mode_t circular_clock_get_badge_mode(void) {
    return s_clock.badge_mode;
}

bool circular_clock_get_ampm_mode(void) {
    return s_clock.ampm_mode;
}

uint8_t circular_clock_get_locale_count(void) {
    return locale_data_get_count();
}

esp_err_t circular_clock_get_locale_tag(uint8_t index, char* buf, size_t size) {
    if (!buf || size < 16) {
        return ESP_ERR_INVALID_ARG;
    }
    
    const locale_definition_t* locale = locale_data_get_by_index(index);
    if (!locale) {
        ESP_LOGE(TAG, "Invalid locale index: %d", index);
        return ESP_ERR_INVALID_ARG;
    }
    
    strncpy(buf, locale->tag, size - 1);
    buf[size - 1] = '\0';
    return ESP_OK;
}

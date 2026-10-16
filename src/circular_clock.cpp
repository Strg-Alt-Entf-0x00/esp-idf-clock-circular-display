/**
 * @file circular_clock.cpp
 * @brief Core controller coordinating dial rings, center cluster, and radio animations
 */

#include "circular_clock.h"
#include "circular_clock_theme.h"
#include "dial_renderer.h"
#include "center_renderer.h"
#include "radio_animator.h"
#include "esp_timer.h"
#include "esp_log.h"
#include <string>

#define LGFX_USE_V1
#include <LovyanGFX.hpp>

static const char* TAG = "circular_clock";

// Declaration from display_hal.cpp
esp_err_t circular_clock_hal_init(const circular_clock_config_t* config);

struct ClockState {
    int hour = 12;
    int minute = 0;
    int second = 0;
    int millisecond = 0;

    int year = 2026;
    int month = 10;
    int day = 16;
    int wday = 5;

    bool custom_date_set = false;
    std::string date_str = "Fr, 16. Okt";

    clock_language_t language = CLOCK_LANG_DE;
    clock_date_format_t date_format = CLOCK_DATE_FMT_ABBREV;
    clock_badge_mode_t badge_mode = CLOCK_BADGE_SMART_AUTO;

    circular_radio_status_t radio_status = CIRCULAR_RADIO_FREE_RUN;
    uint32_t lock_acquired_time_ms = 0;

    bool ampm_mode = false;
    bool is_pm = false;
};

static ClockState s_clock;

static void update_formatted_date_string(ClockState& state) {
    if (state.custom_date_set) return;

    // Weekday names
    static const char* const s_wdays_de_short[] = {"So", "Mo", "Di", "Mi", "Do", "Fr", "Sa"};
    static const char* const s_wdays_de_full[]  = {"Sonntag", "Montag", "Dienstag", "Mittwoch", "Donnerstag", "Freitag", "Samstag"};
    static const char* const s_wdays_en_short[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    static const char* const s_wdays_en_full[]  = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

    // Month names (DIN 5008 German standard & English standard)
    static const char* const s_months_de[] = {"Jan", "Feb", "Mrz", "Apr", "Mai", "Jun", "Jul", "Aug", "Sep", "Okt", "Nov", "Dez"};
    static const char* const s_months_en[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

    int w = (state.wday >= 0 && state.wday < 7) ? state.wday : 0;
    int m_idx = (state.month >= 1 && state.month <= 12) ? (state.month - 1) : 0;

    char buf[48];
    if (state.language == CLOCK_LANG_DE) {
        if (state.date_format == CLOCK_DATE_FMT_NUMERIC) {
            snprintf(buf, sizeof(buf), "%s %02d.%02d", s_wdays_de_short[w], state.day, state.month);
        } else if (state.date_format == CLOCK_DATE_FMT_FULL_WEEKDAY) {
            snprintf(buf, sizeof(buf), "%s, %d. %s", s_wdays_de_full[w], state.day, s_months_de[m_idx]);
        } else { // CLOCK_DATE_FMT_ABBREV
            snprintf(buf, sizeof(buf), "%s, %d. %s", s_wdays_de_short[w], state.day, s_months_de[m_idx]);
        }
    } else { // CLOCK_LANG_EN
        if (state.date_format == CLOCK_DATE_FMT_NUMERIC) {
            snprintf(buf, sizeof(buf), "%s %02d.%02d", s_wdays_en_short[w], state.day, state.month);
        } else if (state.date_format == CLOCK_DATE_FMT_FULL_WEEKDAY) {
            snprintf(buf, sizeof(buf), "%s, %s %d", s_wdays_en_full[w], s_months_en[m_idx], state.day);
        } else { // CLOCK_DATE_FMT_ABBREV
            snprintf(buf, sizeof(buf), "%s, %d %s", s_wdays_en_short[w], state.day, s_months_en[m_idx]);
        }
    }
    state.date_str = buf;
}

esp_err_t circular_clock_init(const circular_clock_config_t* config) {
    ESP_LOGI(TAG, "Initializing 24h Luxury Circular Funkuhr");
    update_formatted_date_string(s_clock);
    return circular_clock_hal_init(config);
}

void circular_clock_set_time(int hour, int minute, int second, int millisecond) {
    s_clock.hour = (hour >= 0 && hour < 24) ? hour : 0;
    s_clock.minute = (minute >= 0 && minute < 60) ? minute : 0;
    s_clock.second = (second >= 0 && second < 60) ? second : 0;
    s_clock.millisecond = (millisecond >= 0 && millisecond < 1000) ? millisecond : 0;
    s_clock.is_pm = (s_clock.hour >= 12);
}

void circular_clock_set_date(int year, int month, int day, int wday) {
    s_clock.year = year;
    s_clock.month = month;
    s_clock.day = day;
    s_clock.wday = wday;
    s_clock.custom_date_set = false;
    update_formatted_date_string(s_clock);
}

void circular_clock_set_date_str(const char* date_str) {
    if (date_str && date_str[0] != '\0') {
        s_clock.custom_date_set = true;
        s_clock.date_str = date_str;
    } else {
        s_clock.custom_date_set = false;
        update_formatted_date_string(s_clock);
    }
}

void circular_clock_set_language(clock_language_t lang) {
    s_clock.language = lang;
    update_formatted_date_string(s_clock);
}

void circular_clock_set_date_format(clock_date_format_t fmt) {
    s_clock.date_format = fmt;
    update_formatted_date_string(s_clock);
}

void circular_clock_set_badge_mode(clock_badge_mode_t mode) {
    s_clock.badge_mode = mode;
}

void circular_clock_set_radio_status(circular_radio_status_t status) {
    if (s_clock.radio_status != CIRCULAR_RADIO_LOCKED && status == CIRCULAR_RADIO_LOCKED) {
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
    float h_prog = (s_clock.hour + s_clock.minute / 60.0f) * 15.0f;
    float m_prog = (s_clock.minute + s_clock.second / 60.0f) * 6.0f;
    g_radio_animator.triggerCalibrationSweep(h_prog, m_prog);
    ESP_LOGI(TAG, "Triggered Atomic Calibration Sweep");
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
    dial_renderer_draw(sprite, s_clock.hour, s_clock.minute, s_clock.second, s_clock.millisecond, s_clock.radio_status);

    // 4. Calculate lock recent state (10s duration for Smart Auto)
    bool lock_recent = (s_clock.radio_status == CIRCULAR_RADIO_LOCKED) &&
                       ((now_ms - s_clock.lock_acquired_time_ms) < 10000);

    // 5. Render compact watchmaker center cluster
    center_renderer_draw(sprite, s_clock.hour, s_clock.minute, s_clock.second,
                         s_clock.year, s_clock.month, s_clock.day,
                         s_clock.date_str.c_str(),
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

/**
 * @file center_renderer.cpp
 * @brief Implementation of the compact center cluster
 *
 *  - CY-36: Circular Badge (Smart Auto Antenna ▲))) / "24" / AM-PM)
 *  - CY-8:  Digital clock HH : MM : SS (16pt font, 18px horizontal step)
 *  - CY+14: Date readout (DIN German / English formatted)
 *  - CY+45: Approximate calendar-day lunar phase
 */

#include "center_renderer.h"
#include "circular_clock_theme.h"
#include "fonts/font_config.h"
#include "radio_animator.h"
#include <cmath>
#include <cstdio>

using namespace clock_theme;

void center_renderer_draw(lgfx::LGFX_Sprite* sprite,
                          int hour, int minute, int second,
                          const char* date_str,
                          bool time_valid, bool date_valid,
                          bool moon_phase_valid, float moon_phase,
                          circular_radio_status_t radio_status,
                          clock_badge_mode_t badge_mode,
                          bool lock_recently_acquired,
                          bool ampm_mode, bool is_pm) {
    if (!sprite) return;

    sprite->setTextDatum(lgfx::middle_center);

    // ========================================================================
    // 1. Top Circular Badge (CY - 36): Antenna Symbol or Mode
    // ========================================================================
    int badge_x = CX;
    int badge_y = CENTER_BADGE_Y;
    int badge_r = CENTER_BADGE_RADIUS;

    sprite->fillCircle(badge_x, badge_y, badge_r, lgfx::color565(12, 18, 28));
    sprite->drawCircle(badge_x, badge_y, badge_r, lgfx::color565(55, 80, 115));

    bool show_antenna = false;
    if (badge_mode == CLOCK_BADGE_ALWAYS_ANTENNA) {
        show_antenna = true;
    } else if (badge_mode == CLOCK_BADGE_ALWAYS_MODE) {
        show_antenna = false;
    } else {
        // CLOCK_BADGE_SMART_AUTO
        show_antenna = (radio_status == CIRCULAR_RADIO_SEARCHING) ||
                       (radio_status == CIRCULAR_RADIO_LOCKED &&
                        lock_recently_acquired);
    }

    if (show_antenna) {
        // Draw Precision Antenna Symbol ▲)))
        const bool is_locked = (radio_status == CIRCULAR_RADIO_LOCKED);
        const bool is_searching = (radio_status == CIRCULAR_RADIO_SEARCHING);
        const uint16_t tower_color = is_locked ? COLOR_RADIO_LOCKED :
                                     (is_searching ? COLOR_RADIO_SEARCH :
                                                     COLOR_RADIO_QUARTZ);

        // Mast position
        int mast_x = badge_x - 3;
        int mast_base_y = badge_y + 4;
        int mast_top_y  = badge_y - 2;

        // Vertical mast & foot
        sprite->drawLine(mast_x, mast_top_y, mast_x, mast_base_y, tower_color);
        sprite->drawLine(mast_x - 1, mast_base_y, mast_x + 1, mast_base_y, tower_color);

        // Mast tip triangle ▲
        sprite->fillTriangle(mast_x - 2, mast_top_y + 1,
                             mast_x + 2, mast_top_y + 1,
                             mast_x,     mast_top_y - 3,
                             tower_color);

        // Wave arcs )))
        int wave_phase = g_radio_animator.getAntennaWavePhase();

        // Wave 1 (r=3)
        if (is_locked || (is_searching && wave_phase >= 1)) {
            sprite->drawArc(mast_x, mast_top_y - 1, 3, 4, -45.0f, 45.0f, tower_color);
        }
        // Wave 2 (r=6)
        if (is_locked || (is_searching && wave_phase >= 2)) {
            sprite->drawArc(mast_x, mast_top_y - 1, 6, 7, -45.0f, 45.0f, tower_color);
        }
        // Wave 3 (r=9)
        if (is_locked || (is_searching && wave_phase >= 3)) {
            sprite->drawArc(mast_x, mast_top_y - 1, 8, 9, -40.0f, 40.0f, tower_color);
        }
    } else {
        // Display "24" or AM/PM
        sprite->setFont(FONT_SMALL);
        sprite->setTextColor(lgfx::color565(170, 205, 255));

        if (radio_status == CIRCULAR_RADIO_UNKNOWN) {
            sprite->drawString("?", badge_x, badge_y);
        } else if (radio_status == CIRCULAR_RADIO_FREE_RUN) {
            sprite->drawString("H", badge_x, badge_y);
        } else if (ampm_mode) {
            sprite->drawString(is_pm ? "PM" : "AM", badge_x, badge_y);
        } else {
            sprite->drawString("24", badge_x, badge_y);
        }
    }

    // ========================================================================
    // 2. Center Digital Time: HH : MM : SS (16pt Font, 18px horizontal step)
    // ========================================================================
    char h_str[4], m_str[4], s_str[4];
    int display_hour = hour;
    if (time_valid && ampm_mode) {
        display_hour = hour % 12;
        if (display_hour == 0) display_hour = 12;
    }
    if (time_valid) {
        snprintf(h_str, sizeof(h_str), "%02d", display_hour);
        snprintf(m_str, sizeof(m_str), "%02d", minute);
        snprintf(s_str, sizeof(s_str), "%02d", second);
    } else {
        snprintf(h_str, sizeof(h_str), "--");
        snprintf(m_str, sizeof(m_str), "--");
        snprintf(s_str, sizeof(s_str), "--");
    }

    sprite->setFont(FONT_DIGITAL);

    // Hours (Crimson Red — matching hour comet)
    sprite->setTextColor(COLOR_HOUR_TEXT);
    sprite->drawString(h_str, CX - 36, CENTER_TIME_Y);

    // First Colon (Subtle Slate Gray)
    sprite->setTextColor(COLOR_TEXT_COLON);
    sprite->drawString(":", CX - 18, CENTER_TIME_Y);

    // Minutes (Electric Cyan — matching minute comet)
    sprite->setTextColor(COLOR_MINUTE_TEXT);
    sprite->drawString(m_str, CX, CENTER_TIME_Y);

    // Second Colon (Subtle Slate Gray)
    sprite->setTextColor(COLOR_TEXT_COLON);
    sprite->drawString(":", CX + 18, CENTER_TIME_Y);

    // Seconds (Luminous Warm Amber — matching second indicator)
    sprite->setTextColor(COLOR_SECOND_AMBER);
    sprite->drawString(s_str, CX + 36, CENTER_TIME_Y);

    // ========================================================================
    // 3. Date Readout (CY + 14)
    // ========================================================================
    const bool calendar_display_valid = time_valid && date_valid;
    if (calendar_display_valid && date_str && date_str[0] != '\0') {
        sprite->setFont(FONT_SMALL);
        sprite->setTextColor(lgfx::color565(170, 185, 205));
        sprite->drawString(date_str, CX, CENTER_DATE_Y);
    }

    // ========================================================================
    // 4. Approximate Moon Phase (Centered, Radius 10, Swiss Horology Earthshine)
    // ========================================================================
    int moon_x = CX;
    int moon_y = CENTER_MOON_Y;
    int moon_r = CENTER_MOON_RADIUS;

    if (calendar_display_valid && moon_phase_valid) {
        // The phase is computed by the clock model only when a valid date is set.
        sprite->fillCircle(moon_x, moon_y, moon_r, COLOR_MOON_DARK);
        const uint16_t moon_lit = COLOR_MOON_LIT;

        if (moon_phase < 0.015f || moon_phase > 0.985f) {
            // New Moon: completely in earthshine shadow.
        } else if (fabsf(moon_phase - 0.5f) < 0.015f) {
            // Full Moon: completely illuminated.
            sprite->fillCircle(moon_x, moon_y, moon_r, moon_lit);
        } else {
            const float k = cosf(moon_phase * 2.0f * (float)M_PI);
            for (int row = -moon_r; row <= moon_r; ++row) {
                const float w2 = (float)(moon_r * moon_r - row * row);
                const float w = sqrtf(w2);
                const float lit_start_f = moon_phase < 0.5f ? w * k : -w;
                const float lit_end_f = moon_phase < 0.5f ? w : -w * k;
                const int x_start = (int)roundf((float)moon_x + lit_start_f);
                const int x_end = (int)roundf((float)moon_x + lit_end_f);
                const int len = x_end - x_start + 1;
                if (len > 0) {
                    sprite->drawFastHLine(x_start, moon_y + row, len, moon_lit);
                }
            }
        }
        sprite->drawCircle(moon_x, moon_y, moon_r, COLOR_MOON_BORDER);
    }
}

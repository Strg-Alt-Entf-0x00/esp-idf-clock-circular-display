/**
 * @file dial_renderer.cpp
 * @brief Professional dual-chapter dial with Trennschiene, comets, and radar sweep
 */

#include "dial_renderer.h"
#include "circular_clock_theme.h"
#include "fonts/font_config.h"
#include "radio_animator.h"
#include <cmath>
#include <cstdio>

using namespace clock_theme;

void dial_renderer_draw(lgfx::LGFX_Sprite* sprite, int hour, int minute, int second,
                        int millisecond, bool time_valid,
                        circular_radio_status_t radio_status) {
    if (!sprite) return;

    float sec_float = time_valid ? (float)second + (float)millisecond / 1000.0f : 0.0f;
    float min_float = time_valid ? (float)minute + sec_float / 60.0f : 0.0f;
    float hr_float  = time_valid ? (float)hour   + min_float / 60.0f : 0.0f;

    // ------------------------------------------------------------------------
    // 1. Outer Ring: 24-Hour Comet Pointer (Fading Tail on R116..119)
    // ------------------------------------------------------------------------
    // 24h = 360°, so 1h = 15°
    if (time_valid) {
        float h_progress = hr_float * 15.0f;
        float h_head_ang = -90.0f + h_progress + g_radio_animator.getHourAngleOffset();
        float h_tail_len = 35.0f; // 35° tail length (~2.3 hours)
        float h_step = 2.0f;

        for (float a = 0.0f; a < h_tail_len; a += h_step) {
            float start_ang = h_head_ang - h_tail_len + a;
            float end_ang = start_ang + h_step + 0.5f;
            if (end_ang > h_head_ang) end_ang = h_head_ang;

            float ratio = a / h_tail_len; // 0.0 at tail end, 1.0 at head
            float intensity = powf(ratio, 1.8f);

            // Crimson Red comet fade: tail is very dark, tip is glowing bright red
            uint8_t r = (uint8_t)(30.0f + intensity * 225.0f);
            uint8_t g = (uint8_t)(intensity * 55.0f);
            uint8_t b = (uint8_t)(intensity * 35.0f);

            sprite->fillArc(CX, CY, RADIUS_HOUR_INNER, RADIUS_HOUR_OUTER,
                            start_ang, end_ang, lgfx::color565(r, g, b));
        }

        // Hour head bead (Glowing tip)
        float h_rad = h_head_ang * (float)M_PI / 180.0f;
        float h_mid_r = (RADIUS_HOUR_INNER + RADIUS_HOUR_OUTER) / 2.0f;
        int h_x = CX + (int)(cosf(h_rad) * h_mid_r);
        int h_y = CY + (int)(sinf(h_rad) * h_mid_r);
        sprite->fillCircle(h_x, h_y, 3, COLOR_HOUR_BEAD);

        // ------------------------------------------------------------------------
        // 2. Inner Ring: 60-Minute Comet Pointer (Fading Tail on R71..74)
        // ------------------------------------------------------------------------
        // 60m = 360°, so 1m = 6°
        float m_progress = min_float * 6.0f;
        float m_head_ang = -90.0f + m_progress + g_radio_animator.getMinuteAngleOffset();
        float m_tail_len = 45.0f; // 45° tail length (~7.5 minutes)
        float m_step = 2.0f;

        for (float a = 0.0f; a < m_tail_len; a += h_step) {
            float start_ang = m_head_ang - m_tail_len + a;
            float end_ang = start_ang + m_step + 0.5f;
            if (end_ang > m_head_ang) end_ang = m_head_ang;

            float ratio = a / m_tail_len;
            float intensity = powf(ratio, 1.8f);

            // Cyan-Blue comet fade: tail is dark, tip is electric cyan
            uint8_t r = (uint8_t)(intensity * 40.0f);
            uint8_t g = (uint8_t)(25.0f + intensity * 200.0f);
            uint8_t b = (uint8_t)(45.0f + intensity * 210.0f);

            sprite->fillArc(CX, CY, RADIUS_MINUTE_INNER, RADIUS_MINUTE_OUTER,
                            start_ang, end_ang, lgfx::color565(r, g, b));
        }

        // Minute head bead (Glowing tip pointing outward at minute numbers)
        float m_rad = m_head_ang * (float)M_PI / 180.0f;
        float m_mid_r = (RADIUS_MINUTE_INNER + RADIUS_MINUTE_OUTER) / 2.0f;
        int m_x = CX + (int)(cosf(m_rad) * m_mid_r);
        int m_y = CY + (int)(sinf(m_rad) * m_mid_r);
        sprite->fillCircle(m_x, m_y, 3, COLOR_MINUTE_BEAD);
    }

    // ------------------------------------------------------------------------
    // 3. Full 24h Dial Scale (Outer Ring, Radius 106)
    // Top = 24 (Midnight), Right = 6, Bottom = 12 (Noon), Left = 18
    // ------------------------------------------------------------------------
    sprite->setTextDatum(lgfx::middle_center);
    sprite->setFont(FONT_SCALE);
    sprite->setTextSize(1.0f);

    for (int h = 0; h < 24; h++) {
        float angle_deg = h * 15.0f - 90.0f;
        float angle_rad = angle_deg * (float)M_PI / 180.0f;
        float cx = CX + cosf(angle_rad) * (float)RADIUS_SCALE_HOURS;
        float cy = CY + sinf(angle_rad) * (float)RADIUS_SCALE_HOURS;

        int num = (h == 0) ? 24 : h;
        bool is_current_hour = time_valid && (hour == h);

        if (is_current_hour) {
            sprite->setTextColor(COLOR_TEXT_ACTIVE);
        } else {
            sprite->setTextColor(COLOR_TEXT_DIM);
        }

        char num_str[4];
        snprintf(num_str, sizeof(num_str), "%d", num);
        sprite->drawString(num_str, (int)cx, (int)cy);
    }

    // ------------------------------------------------------------------------
    // 4. Trennschiene: Minute Divider Track (Radius 94..100)
    // Includes Funkuhr Radar-Sweep during SEARCHING state
    // ------------------------------------------------------------------------
    bool is_searching = (radio_status == CIRCULAR_RADIO_SEARCHING);
    float radar_sweep_head = g_radio_animator.getRadarAngle() - 90.0f;

    // Draw radar sweep phosphor arc on the Trennschiene
    if (is_searching) {
        float sweep_len = 45.0f; // 45° glowing phosphor tail
        float sweep_step = 2.5f;
        for (float a = 0.0f; a < sweep_len; a += sweep_step) {
            float sa = radar_sweep_head - sweep_len + a;
            float ea = sa + sweep_step + 0.5f;
            if (ea > radar_sweep_head) ea = radar_sweep_head;

            float ratio = a / sweep_len;
            float intensity = powf(ratio, 2.0f);

            uint8_t r = (uint8_t)(intensity * 20.0f);
            uint8_t g = (uint8_t)(intensity * 230.0f);
            uint8_t b = (uint8_t)(intensity * 255.0f);

            sprite->fillArc(CX, CY, 94, 98, sa, ea, lgfx::color565(r, g, b));
        }

        // Radar beam leading edge line
        float r_rad = radar_sweep_head * (float)M_PI / 180.0f;
        float rc = cosf(r_rad);
        float rs = sinf(r_rad);
        sprite->drawLine(CX + (int)roundf(rc * 92.0f), CY + (int)roundf(rs * 92.0f),
                         CX + (int)roundf(rc * 100.0f), CY + (int)roundf(rs * 100.0f),
                         lgfx::color565(120, 255, 255));
    }

    // ------------------------------------------------------------------------
    // Pac-Man Dot Consumption & Cascading Regeneration Wave
    // ------------------------------------------------------------------------
    bool is_regenerating = time_valid && (second == 0);
    float regen_wave_pos = 0.0f;
    if (is_regenerating) {
        // Clockwise cascading wave around the dial during second 0 (0..650ms)
        regen_wave_pos = ((float)millisecond / 650.0f) * 60.0f;
    }

    // Draw ticks & dots on Trennschiene
    for (int m = 0; m < 60; m++) {
        float m_deg = m * 6.0f - 90.0f;
        float m_rad = m_deg * (float)M_PI / 180.0f;
        float c = cosf(m_rad);
        float s = sinf(m_rad);

        // Radar phosphor illumination boost
        float phosphor_boost = 0.0f;
        if (is_searching) {
            float d_ang = fmodf(radar_sweep_head - m_deg + 360.0f, 360.0f);
            if (d_ang >= 0.0f && d_ang < 45.0f) {
                phosphor_boost = (45.0f - d_ang) / 45.0f;
            }
        }

        if (m % 15 == 0) {
            // Quarter marks (0, 15, 30, 45 min): prominent tick line across boundary (R92..100)
            int x1 = CX + (int)roundf(c * 92.0f);
            int y1 = CY + (int)roundf(s * 92.0f);
            int x2 = CX + (int)roundf(c * 100.0f);
            int y2 = CY + (int)roundf(s * 100.0f);

            uint16_t tick_col = COLOR_TICK_QUARTER;
            if (phosphor_boost > 0.0f) {
                tick_col = lerp_color565(tick_col, lgfx::color565(200, 255, 255), phosphor_boost);
            }
            sprite->drawLine(x1, y1, x2, y2, tick_col);
        } else if (m % 5 == 0) {
            // 5-minute marks: crisp tick line (R93.5..98.5)
            int x1 = CX + (int)roundf(c * 93.5f);
            int y1 = CY + (int)roundf(s * 93.5f);
            int x2 = CX + (int)roundf(c * 98.5f);
            int y2 = CY + (int)roundf(s * 98.5f);

            uint16_t tick_col = COLOR_TICK_FIVE;
            if (phosphor_boost > 0.0f) {
                tick_col = lerp_color565(tick_col, lgfx::color565(80, 255, 255), phosphor_boost);
            }
            sprite->drawLine(x1, y1, x2, y2, tick_col);
        } else {
            // Intermediate 1-minute dots: "Pac-Man" consumption mechanic
            bool dot_visible = false;
            float spawn_flash = 0.0f;

            if (!time_valid) {
                dot_visible = false;
            } else if (is_regenerating) {
                // Minute turnaround: dots respawn in a smooth cascading clockwise wave
                if ((float)m <= regen_wave_pos) {
                    dot_visible = true;
                    float dist = regen_wave_pos - (float)m;
                    if (dist >= 0.0f && dist < 6.0f) {
                        spawn_flash = (6.0f - dist) / 6.0f;
                    }
                }
            } else {
                // The amber second pip has consumed dots up to current second
                // Dots ahead (> second) remain visible waiting to be consumed
                if (m > second) {
                    dot_visible = true;
                }
            }

            if (dot_visible) {
                int px = CX + (int)roundf(c * (float)RADIUS_DIVIDER_TRACK);
                int py = CY + (int)roundf(s * (float)RADIUS_DIVIDER_TRACK);

                uint16_t dot_col = COLOR_DOT_MINUTE;
                if (spawn_flash > 0.0f) {
                    // Soft neon bloom when dot spawns back
                    dot_col = lerp_color565(COLOR_DOT_MINUTE, lgfx::color565(180, 240, 255), spawn_flash);
                } else if (phosphor_boost > 0.0f) {
                    dot_col = lerp_color565(dot_col, lgfx::color565(0, 255, 230), phosphor_boost);
                }
                sprite->drawPixel(px, py, dot_col);
            }
        }
    }

    // ------------------------------------------------------------------------
    // 5. Interactive Amber Second Pip (Orbits directly on the Trennschiene R97)
    // ------------------------------------------------------------------------
    if (time_valid) {
        float sec_deg = second * 6.0f - 90.0f;
        float sec_rad = sec_deg * (float)M_PI / 180.0f;
        int sec_x = CX + (int)(cosf(sec_rad) * (float)RADIUS_DIVIDER_TRACK);
        int sec_y = CY + (int)(sinf(sec_rad) * (float)RADIUS_DIVIDER_TRACK);

        if (second % 15 == 0) {
            // Quarter hit: large radiant pulse
            sprite->drawCircle(sec_x, sec_y, 5, lgfx::color565(255, 160, 20));
            sprite->fillCircle(sec_x, sec_y, 3, COLOR_SECOND_GLOW);
        } else if (second % 5 == 0) {
            // 5-min mark hit: glowing highlight circle
            sprite->drawCircle(sec_x, sec_y, 4, lgfx::color565(210, 140, 25));
            sprite->fillCircle(sec_x, sec_y, 3, lgfx::color565(255, 210, 40));
        } else {
            // Standard crisp amber pip
            sprite->fillCircle(sec_x, sec_y, 2, COLOR_SECOND_AMBER);
        }
    }

    // ------------------------------------------------------------------------
    // 6. 60-Minute Dial Scale (Inner Ring, Radius 85)
    // 12 numbers at 30° intervals: 60 (top), 05, 10, 15 ... 30 (bottom) ... 55
    // ------------------------------------------------------------------------
    sprite->setFont(FONT_SMALL);

    for (int i = 0; i < 12; i++) {
        float angle_deg = i * 30.0f - 90.0f;
        float angle_rad = angle_deg * (float)M_PI / 180.0f;
        float cx = CX + cosf(angle_rad) * (float)RADIUS_SCALE_MINUTES;
        float cy = CY + sinf(angle_rad) * (float)RADIUS_SCALE_MINUTES;

        char min_str[4];
        if (i == 0) {
            snprintf(min_str, sizeof(min_str), "60");
        } else {
            snprintf(min_str, sizeof(min_str), "%02d", i * 5);
        }

        bool is_active_min = time_valid && (minute / 5 == i);
        if (is_active_min) {
            sprite->setTextColor(COLOR_TEXT_ACTIVE);
        } else {
            sprite->setTextColor(COLOR_TEXT_DIM);
        }

        sprite->drawString(min_str, (int)cx, (int)cy);
    }
}

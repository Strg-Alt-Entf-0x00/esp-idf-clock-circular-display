/**
 * @file circular_clock_theme.h
 * @brief Dial geometry, radii, and luxury watchmaker color definitions
 *
 * Implements the verified 24h luxury dual-chapter circular dial:
 *  - R 116..119: 24h Red Comet (Hours)
 *  - R 106:      24h Hour Scale (10pt Noto Sans, 1..24)
 *  - R 97:       Divider Track (Ticks 94..100, intermediate dots, radar sweep)
 *  - R 97:       Interactive Amber Second Pip (Pulsing on 5m and 15m)
 *  - R 85:       60m Minute Scale (8pt Noto Sans, 60, 05..55, 30)
 *  - R 71..74:   60m Cyan Comet (Minutes)
 *  - R <= 48:    Compact Center (Badge CY-36, Time CY-8, Date CY+14, Moon CY+34)
 */

#pragma once

#include <stdint.h>

namespace clock_theme {

// ----------------------------------------------------------------------------
// Display Geometry
// ----------------------------------------------------------------------------
constexpr int DISPLAY_WIDTH  = 240;
constexpr int DISPLAY_HEIGHT = 240;
constexpr int CX             = 120;
constexpr int CY             = 120;
constexpr int RADIUS_MAX     = 120;

// ----------------------------------------------------------------------------
// Chapter Ring Radii (Outside-In) - Balanced Watchmaker Proportions
// ----------------------------------------------------------------------------
constexpr int RADIUS_HOUR_OUTER     = 119; // 24h Hour comet outer track
constexpr int RADIUS_HOUR_INNER     = 116; // 24h Hour comet inner track
constexpr int RADIUS_SCALE_HOURS    = 108; // 24h Hour numbers (1..24) - airy spacing
constexpr int RADIUS_DIVIDER_TRACK  = 96;  // Trennschiene (Ticks + Seconds + Radar)
constexpr int RADIUS_SCALE_MINUTES  = 82;  // 60m Minute numbers (60, 05..55) - airy spacing
constexpr int RADIUS_MINUTE_OUTER   = 72;  // 60m Minute comet outer track
constexpr int RADIUS_MINUTE_INNER   = 69;  // 60m Minute comet inner track

// Center Cluster Layout (Vertical Offsets from CY=120)
constexpr int CENTER_BADGE_Y        = CY - 36; // Radio / 24h circular badge
constexpr int CENTER_BADGE_RADIUS   = 10;      // Badge radius
constexpr int CENTER_TIME_Y         = CY - 8;  // Digital HH : MM : SS (16pt)
constexpr int CENTER_DATE_Y         = CY + 14; // Date string (8pt)
constexpr int CENTER_MOON_Y         = CY + 45; // Astronomical Moon (Radius 10, balanced between Date & lower ring)
constexpr int CENTER_MOON_RADIUS    = 10;

// ----------------------------------------------------------------------------
// Color Palette (RGB565)
// ----------------------------------------------------------------------------
constexpr uint16_t COLOR_BG           = 0x0000; // Deep obsidian black

// Typography
constexpr uint16_t COLOR_TEXT_ACTIVE  = 0xFFFF; // Pure brilliant white
constexpr uint16_t COLOR_TEXT_DIM     = 0x39E7; // Muted dark slate gray
constexpr uint16_t COLOR_TEXT_COLON   = 0x7BEF; // Subtle separator slate gray

// Hour Comet & Digital Hour (Crimson Red)
constexpr uint16_t COLOR_HOUR_HEAD    = 0xF800; // Pure red
constexpr uint16_t COLOR_HOUR_BEAD    = 0xFC45; // Glowing crimson-orange bead (255, 60, 40)
constexpr uint16_t COLOR_HOUR_TEXT    = 0xFA47; // Crisp digital red (255, 75, 60)

// Minute Comet & Digital Minute (Electric Cyan)
constexpr uint16_t COLOR_MINUTE_HEAD  = 0x06BF; // Bright cyan
constexpr uint16_t COLOR_MINUTE_BEAD  = 0x06FF; // Glowing electric cyan bead (0, 220, 255)
constexpr uint16_t COLOR_MINUTE_TEXT  = 0x06FF; // Crisp digital cyan (0, 220, 255)

// Seconds Pip & Digital Second (Luminous Warm Amber)
constexpr uint16_t COLOR_SECOND_AMBER = 0xFD62; // Pure warm amber (255, 175, 20)
constexpr uint16_t COLOR_SECOND_GLOW  = 0xFFE8; // Intense amber-gold (255, 235, 80)
constexpr uint16_t COLOR_SECOND_RING  = 0xFA62; // Highlight pulse ring

// Trennschiene Divider Track
constexpr uint16_t COLOR_TICK_QUARTER = 0x8DFB; // Bright ice blue (140, 220, 255)
constexpr uint16_t COLOR_TICK_FIVE    = 0x4BBA; // Steel blue (70, 115, 160)
constexpr uint16_t COLOR_DOT_MINUTE   = 0x3CEF; // Crisp steel slate bead (60, 95, 135)

// Radio & Radar Oscilloscope Colors
constexpr uint16_t COLOR_RADAR_SWEEP  = 0x07FF; // Electric emerald-cyan scan beam
constexpr uint16_t COLOR_RADIO_LOCKED = 0x07FF; // Confirmed atomic lock
constexpr uint16_t COLOR_RADIO_SEARCH = 0xFDA0; // Seeking radio wave amber
constexpr uint16_t COLOR_RADIO_QUARTZ = 0x8410; // Quartz / Free-run gray

// Moon Phase (Swiss Watchmaker Earthshine & Pearl Silver)
constexpr uint16_t COLOR_MOON_DARK    = 0x1185; // Deep midnight slate earthshine (18, 26, 38)
constexpr uint16_t COLOR_MOON_BORDER  = 0x328A; // Subtle steel bezel rim (50, 70, 95)
constexpr uint16_t COLOR_MOON_LIT     = 0xFFDF; // Brilliant lunar pearl white (250, 252, 255)

/**
 * @brief Convert HSV to RGB565
 */
uint16_t hsv_to_rgb565(float h, float s, float v);

/**
 * @brief Linear interpolation between two RGB565 colors
 */
uint16_t lerp_color565(uint16_t c1, uint16_t c2, float t);

} // namespace clock_theme

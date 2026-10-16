/**
 * @file theme.cpp
 * @brief Color conversions and mathematical utilities for luxury clock dial
 */

#include "circular_clock_theme.h"
#include <cmath>

#define LGFX_USE_V1
#include <LovyanGFX.hpp>

namespace clock_theme {

uint16_t hsv_to_rgb565(float h, float s, float v) {
    float h_mod = fmodf(h, 360.0f);
    if (h_mod < 0) h_mod += 360.0f;

    int hi = (int)(h_mod / 60.0f) % 6;
    float f = (h_mod / 60.0f) - (float)hi;
    float p = v * (1.0f - s);
    float q = v * (1.0f - f * s);
    float t = v * (1.0f - (1.0f - f) * s);

    float r = 0, g = 0, b = 0;
    switch (hi) {
        case 0: r = v; g = t; b = p; break;
        case 1: r = q; g = v; b = p; break;
        case 2: r = p; g = v; b = t; break;
        case 3: r = p; g = q; b = v; break;
        case 4: r = t; g = p; b = v; break;
        case 5: r = v; g = p; b = q; break;
    }

    uint8_t r8 = (uint8_t)(r * 255.0f);
    uint8_t g8 = (uint8_t)(g * 255.0f);
    uint8_t b8 = (uint8_t)(b * 255.0f);

    return lgfx::color565(r8, g8, b8);
}

uint16_t lerp_color565(uint16_t c1, uint16_t c2, float t) {
    if (t <= 0.0f) return c1;
    if (t >= 1.0f) return c2;

    uint8_t r1 = (c1 >> 11) & 0x1F;
    uint8_t g1 = (c1 >> 5)  & 0x3F;
    uint8_t b1 = c1         & 0x1F;

    uint8_t r2 = (c2 >> 11) & 0x1F;
    uint8_t g2 = (c2 >> 5)  & 0x3F;
    uint8_t b2 = c2         & 0x1F;

    uint8_t r = (uint8_t)(r1 + (r2 - r1) * t);
    uint8_t g = (uint8_t)(g1 + (g2 - g1) * t);
    uint8_t b = (uint8_t)(b1 + (b2 - b1) * t);

    return (uint16_t)((r << 11) | (g << 5) | b);
}

} // namespace clock_theme

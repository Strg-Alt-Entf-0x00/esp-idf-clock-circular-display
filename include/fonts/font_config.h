/**
 * @file font_config.h
 * @brief Font configuration for luxury circular clock
 *
 * Uses pre-rendered high-readability Noto Sans fonts at native pixel sizes:
 *   - 8pt:  Date string, 60m minute dial markers (60, 05..55), badges
 *   - 10pt: 24h dial markers (1..24)
 *   - 16pt: Digital time readout (HH : MM : SS)
 */

#pragma once

#ifndef U8G2_FONT_SECTION
#define U8G2_FONT_SECTION(name)
#endif

#define LGFX_USE_V1
#include <LovyanGFX.hpp>

#include "fonts/noto_sans_8.h"
#include "fonts/noto_sans_10.h"
#include "fonts/noto_sans_16.h"

// Extern instances defined in src/font_instances.cpp
extern const lgfx::U8g2font noto_sans_8_lgfx;
extern const lgfx::U8g2font noto_sans_10_lgfx;
extern const lgfx::U8g2font noto_sans_16_lgfx;

#define FONT_SMALL    (&noto_sans_8_lgfx)   // 8pt: date, 60m minute markers, AM/PM
#define FONT_SCALE    (&noto_sans_10_lgfx)  // 10pt: 24h hour scale numbers
#define FONT_DIGITAL  (&noto_sans_16_lgfx)  // 16pt: digital time HH : MM : SS

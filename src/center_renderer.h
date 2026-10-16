/**
 * @file center_renderer.h
 * @brief Renders the compact center cluster (badge, digital time, date, astronomical moon)
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "radio_sync.h"
#include "circular_clock.h"

#define LGFX_USE_V1
#include <LovyanGFX.hpp>

void center_renderer_draw(lgfx::LGFX_Sprite* sprite,
                          int hour, int minute, int second,
                          int year, int month, int day,
                          const char* date_str,
                          circular_radio_status_t radio_status,
                          clock_badge_mode_t badge_mode,
                          bool lock_recently_acquired,
                          bool ampm_mode, bool is_pm);

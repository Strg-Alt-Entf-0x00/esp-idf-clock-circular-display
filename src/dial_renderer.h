/**
 * @file dial_renderer.h
 * @brief Renders the outer and inner comet chapter rings, 24h & 60m scales, and Trennschiene
 */

#pragma once

#include <stdint.h>
#include "radio_sync.h"

#define LGFX_USE_V1
#include <LovyanGFX.hpp>

void dial_renderer_draw(lgfx::LGFX_Sprite* sprite, int hour, int minute, int second,
                        int millisecond, bool time_valid,
                        circular_radio_status_t radio_status);

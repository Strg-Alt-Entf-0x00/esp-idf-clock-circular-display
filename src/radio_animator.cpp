/**
 * @file radio_animator.cpp
 * @brief Implementation of radar and synchronization transition effects
 */

#include "radio_animator.h"
#include <cmath>

RadioAnimator g_radio_animator;

RadioAnimator::RadioAnimator()
    : m_last_time_ms(0),
      m_radar_angle(0.0f),
      m_antenna_wave_phase(0),
      m_calibrating(false),
      m_calib_start_ms(0),
      m_calib_h_start_deg(0.0f),
      m_calib_m_start_deg(0.0f),
      m_hour_offset_deg(0.0f),
      m_minute_offset_deg(0.0f) {}

void RadioAnimator::update(uint32_t time_ms) {
    if (m_last_time_ms == 0) {
        m_last_time_ms = time_ms;
    }
    float dt = (time_ms - m_last_time_ms) / 1000.0f;
    m_last_time_ms = time_ms;
    if (dt < 0.0f || dt > 1.0f) dt = 0.033f;

    // 1. Radar sweep rotation (360° every 1.2 seconds = 300°/s)
    m_radar_angle += 300.0f * dt;
    if (m_radar_angle >= 360.0f) {
        m_radar_angle = fmodf(m_radar_angle, 360.0f);
    }

    // 2. Antenna wave pulse phase (cycles 0 -> 1 -> 2 -> 3 every 300ms)
    uint32_t wave_cycle = time_ms % 1200;
    m_antenna_wave_phase = (int)(wave_cycle / 300); // 0, 1, 2, 3

    // 3. Synchronization transition animation
    if (m_calibrating) {
        constexpr uint32_t CALIB_DURATION_MS = 2200;
        uint32_t elapsed = time_ms - m_calib_start_ms;

        if (elapsed >= CALIB_DURATION_MS) {
            m_calibrating = false;
            m_hour_offset_deg = 0.0f;
            m_minute_offset_deg = 0.0f;
        } else {
            float t = (float)elapsed / (float)CALIB_DURATION_MS; // 0.0 -> 1.0
            
            // Cubic Ease-Out with graceful deceleration into lock
            float inv = 1.0f - t;
            float ease = inv * inv * inv; // Starts at 1.0, smoothly approaches 0.0

            // Hour comet does a +360° sweep winding into target
            m_hour_offset_deg = 360.0f * ease;
            
            // Minute comet does a +720° rapid homing sweep winding into target
            m_minute_offset_deg = 720.0f * ease;
        }
    } else {
        m_hour_offset_deg = 0.0f;
        m_minute_offset_deg = 0.0f;
    }
}

void RadioAnimator::triggerCalibrationSweep(float current_h_progress_deg, float current_m_progress_deg) {
    m_calibrating = true;
    m_calib_start_ms = m_last_time_ms;
    m_calib_h_start_deg = current_h_progress_deg;
    m_calib_m_start_deg = current_m_progress_deg;
    m_hour_offset_deg = 360.0f;
    m_minute_offset_deg = 720.0f;
}

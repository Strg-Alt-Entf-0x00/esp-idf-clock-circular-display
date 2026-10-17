/**
 * @file radio_animator.h
 * @brief Animation controller for radar and synchronization transition effects
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

class RadioAnimator {
public:
    RadioAnimator();

    void update(uint32_t time_ms);
    void triggerCalibrationSweep(float current_h_progress_deg, float current_m_progress_deg);

    bool isCalibrating() const { return m_calibrating; }
    float getHourAngleOffset() const { return m_hour_offset_deg; }
    float getMinuteAngleOffset() const { return m_minute_offset_deg; }

    float getRadarAngle() const { return m_radar_angle; }
    int getAntennaWavePhase() const { return m_antenna_wave_phase; }

private:
    uint32_t m_last_time_ms;
    float m_radar_angle;
    int m_antenna_wave_phase;

    bool m_calibrating;
    uint32_t m_calib_start_ms;
    float m_calib_h_start_deg;
    float m_calib_m_start_deg;
    float m_hour_offset_deg;
    float m_minute_offset_deg;
};

extern RadioAnimator g_radio_animator;

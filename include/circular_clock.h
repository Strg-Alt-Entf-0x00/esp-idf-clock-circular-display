/**
 * @file circular_clock.h
 * @brief Public API for the luxury 24h circular Funkuhr on ESP-IDF
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "driver/spi_master.h"
#include "radio_sync.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Display & Hardware Configuration
 */
typedef struct {
    spi_host_device_t spi_host;   /**< SPI Host (e.g. SPI2_HOST) */
    int pin_mosi;                 /**< SPI MOSI GPIO */
    int pin_sclk;                 /**< SPI SCLK GPIO */
    int pin_cs;                   /**< SPI Chip Select GPIO */
    int pin_dc;                   /**< SPI Data/Command GPIO */
    int pin_rst;                  /**< Reset GPIO */
    int pin_bkl;                  /**< Backlight PWM GPIO (-1 if unmanaged) */
    int spi_clock_hz;             /**< SPI Clock in Hz (default 40000000) */
} circular_clock_config_t;

/**
 * @brief Language for weekday and month formatting
 */
typedef enum {
    CLOCK_LANG_DE = 0,            /**< German (e.g. Fr, 16. Okt / Freitag, 16. Okt) */
    CLOCK_LANG_EN = 1             /**< English (e.g. Fri 16 Oct / Friday, 16 Oct) */
} clock_language_t;

/**
 * @brief Date display format
 */
typedef enum {
    CLOCK_DATE_FMT_NUMERIC = 0,       /**< Short numeric: "Fr 16.10" or "Fri 16.10" */
    CLOCK_DATE_FMT_ABBREV = 1,        /**< DIN Abbrev with month name: "Fr, 16. Okt" or "Fri 16 Oct" */
    CLOCK_DATE_FMT_FULL_WEEKDAY = 2   /**< Full weekday: "Freitag, 16. Okt" or "Friday, 16 Oct" */
} clock_date_format_t;

/**
 * @brief Top badge display mode (CY-36)
 */
typedef enum {
    CLOCK_BADGE_SMART_AUTO = 0,       /**< Shows antenna during SEARCH/LOCK, then reverts to clean "24" or "AM"/"PM" */
    CLOCK_BADGE_ALWAYS_ANTENNA = 1,   /**< Always shows the vector antenna ▲))) and signal status */
    CLOCK_BADGE_ALWAYS_MODE = 2       /**< Always shows "24" or "AM"/"PM" */
} clock_badge_mode_t;

/**
 * @brief Initialize circular display hardware, SPI panel, and LovyanGFX sprite buffer
 * @param config Pointer to hardware pin and bus configuration
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t circular_clock_init(const circular_clock_config_t* config);

/**
 * @brief Set current time
 * @param hour Hour (0..23)
 * @param minute Minute (0..59)
 * @param second Second (0..59)
 * @param millisecond Sub-second millisecond (0..999) for fluid animations
 */
void circular_clock_set_time(int hour, int minute, int second, int millisecond);

/**
 * @brief Set calendar date
 * @param year Full year (e.g. 2026)
 * @param month Month (1..12)
 * @param day Day of month (1..31)
 * @param wday Day of week (0=Sun .. 6=Sat)
 */
void circular_clock_set_date(int year, int month, int day, int wday);

/**
 * @brief Set formatted custom date string (overrides automatic date formatting)
 * @param date_str Null-terminated date string (or NULL to revert to auto)
 */
void circular_clock_set_date_str(const char* date_str);

/**
 * @brief Set language for date formatting (German or English)
 */
void circular_clock_set_language(clock_language_t lang);

/**
 * @brief Set date formatting style (Numeric, Abbreviated month, or Full weekday)
 */
void circular_clock_set_date_format(clock_date_format_t fmt);

/**
 * @brief Set top badge display mode (Smart Auto, Always Antenna, Always Mode)
 */
void circular_clock_set_badge_mode(clock_badge_mode_t mode);

/**
 * @brief Set radio synchronization status
 * @param status Status: CIRCULAR_RADIO_SEARCHING, CIRCULAR_RADIO_LOCKED, CIRCULAR_RADIO_FREE_RUN
 */
void circular_clock_set_radio_status(circular_radio_status_t status);

/**
 * @brief Get current radio synchronization status
 */
circular_radio_status_t circular_clock_get_radio_status(void);

/**
 * @brief Trigger an "Atomic Calibration Sweep" animation
 *
 * Simulates the mechanical/electronic homing and settling sweep of both
 * comet pointers into the exact atomic time position (typically executed
 * upon atomic sync lock or system power-up).
 */
void circular_clock_trigger_calibration_sweep(void);

/**
 * @brief Check if an atomic calibration sweep is currently in progress
 */
bool circular_clock_is_calibrating(void);

/**
 * @brief Enable or disable 12-hour AM/PM badge display instead of "24"
 */
void circular_clock_set_ampm_mode(bool enable_ampm, bool is_pm);

/**
 * @brief Set display backlight brightness (0..100 %)
 */
void circular_clock_set_brightness(uint8_t brightness_pct);

/**
 * @brief Render current frame to internal sprite and push to GC9A01 display
 */
/**
 * @brief Render current clock frame to a provided LovyanGFX sprite buffer (240x240)
 *
 * Allows seamless embedding of the clock as an idle screen inside other display
 * managers without managing separate panel handles or duplicate sprite buffers.
 *
 * @param sprite_ptr Pointer to lgfx::LGFX_Sprite
 */
void circular_clock_render_to_sprite(void* sprite_ptr);

void circular_clock_render(void);

/**
 * @brief Get internal LovyanGFX sprite pointer (void* for C compatibility)
 */
void* circular_clock_get_sprite(void);

/**
 * @brief Push current frame buffer to physical display
 */
void circular_clock_push_frame(void);

#ifdef __cplusplus
}
#endif

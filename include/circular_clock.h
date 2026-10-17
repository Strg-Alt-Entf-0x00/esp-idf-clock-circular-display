/**
 * @file circular_clock.h
 * @brief Public API for the 24-hour circular clock display on ESP-IDF
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
 * @brief Date display format
 */

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
 * @brief Initialize circular display hardware, SPI panel, LovyanGFX sprite buffer, and clock settings
 * @param config Pointer to hardware pin and bus configuration, or NULL for sprite-only / headless mode
 *               (e.g. when rendering into an external LovyanGFX sprite buffer without managing hardware)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t circular_clock_init(const circular_clock_config_t* config);

/**
 * The clock API is not internally synchronized. Call setters, status getters,
 * and rendering from one application task; event callbacks should hand data
 * back to that task rather than calling the clock API directly.
 */

/**
 * @brief Set current time
 * @param hour Hour (0..23)
 * @param minute Minute (0..59)
 * @param second Second (0..59)
 * @param millisecond Sub-second millisecond (0..999) for fluid animations
 *
 * Call circular_clock_set_time_valid(true) only after supplying trusted time.
 * Out-of-range values invalidate the displayed time.
 */
void circular_clock_set_time(int hour, int minute, int second, int millisecond);

/**
 * @brief Set whether the supplied clock time is currently valid
 *
 * The display component does not determine time validity or contact a time
 * source. The application must set this explicitly; unknown time is rendered
 * as "--:--:--".
 */
void circular_clock_set_time_valid(bool valid);

/**
 * @brief Set calendar date
 * @param year Full year (e.g. 2026)
 * @param month Month (1..12)
 * @param day Day of month (1..31)
 * @param wday Day of week (0=Sun .. 6=Sat)
 *
 * Invalid calendar values clear the displayed date and moon phase.
 */
void circular_clock_set_date(int year, int month, int day, int wday);

/**
 * @brief Set formatted custom date string (overrides automatic date formatting)
 * @param date_str Caller-formatted string, at most 47 bytes (NULL/empty reverts to auto)
 *
 * Overlong input clears the date and is reported in the log.
 */
void circular_clock_set_date_str(const char* date_str);

/**
 * @brief Set date locale using a BCP 47 language tag
 *
 * Supported locales: de-DE, en-GB, en-US, fr-FR, es-ES, pt-PT, it-IT,
 * da-DK, nb-NO, sv-SE. Bare tags de, en, fr, es, pt, it, da, nb, and sv
 * are accepted as aliases and select the listed default region.
 *
 * Date glyphs are limited to the Latin-1 repertoire supported by the built-in
 * 8px font. Unsupported tags leave the current locale unchanged.
 *
 * @param locale_tag Locale tag (case-insensitive), e.g. "fr-FR"
 * @return ESP_OK, ESP_ERR_INVALID_ARG for NULL/empty tags, or
 *         ESP_ERR_NOT_SUPPORTED for an unsupported tag
 */
esp_err_t circular_clock_set_locale(const char* locale_tag);

/**
 * @brief Get current locale tag
 * @param buf Output buffer (minimum 16 bytes)
 * @param size Buffer size
 * @return ESP_OK, ESP_ERR_INVALID_ARG, or ESP_ERR_INVALID_STATE
 */
esp_err_t circular_clock_get_locale(char* buf, size_t size);

/**
 * @brief Get number of supported locales
 * @return Number of built-in locales (currently 10)
 */
uint8_t circular_clock_get_locale_count(void);

/**
 * @brief Get locale tag by index
 * @param index Locale index (0..count-1)
 * @param buf Output buffer (minimum 16 bytes)
 * @param size Buffer size
 * @return ESP_OK or ESP_ERR_INVALID_ARG
 */
esp_err_t circular_clock_get_locale_tag(uint8_t index, char* buf, size_t size);

/**
 * @brief Set date formatting style (Numeric, Abbreviated month, or Full weekday)
 */
void circular_clock_set_date_format(clock_date_format_t fmt);

/**
 * @brief Get current date format
 */
clock_date_format_t circular_clock_get_date_format(void);

/**
 * @brief Set top badge display mode (Smart Auto, Always Antenna, Always Mode)
 */
void circular_clock_set_badge_mode(clock_badge_mode_t mode);

/**
 * @brief Get current badge mode
 */
clock_badge_mode_t circular_clock_get_badge_mode(void);

/**
 * @brief Set caller-owned time-source status for display
 *
 * This only updates presentation state; it does not acquire, validate, or
 * synchronize time. CIRCULAR_RADIO_FREE_RUN represents caller-reported
 * holdover, while CIRCULAR_RADIO_UNKNOWN means no valid time source is known.
 * The legacy function and enum names are retained for source compatibility.
 */
void circular_clock_set_radio_status(circular_radio_status_t status);

/**
 * @brief Get current radio synchronization status
 */
circular_radio_status_t circular_clock_get_radio_status(void);

/**
 * @brief Trigger the visual synchronization transition animation
 *
 * This is a presentation effect only. It does not calibrate or adjust the
 * supplied clock time.
 */
void circular_clock_trigger_calibration_sweep(void);

/**
 * @brief Check if the synchronization transition animation is in progress
 */
bool circular_clock_is_calibrating(void);

/**
 * @brief Enable or disable 12-hour AM/PM badge display instead of "24"
 */
void circular_clock_set_ampm_mode(bool enable_ampm, bool is_pm);

/**
 * @brief Get current 12h/24h mode
 * @return true if 12h AM/PM mode, false if 24h mode
 */
bool circular_clock_get_ampm_mode(void);

/**
 * @brief Set display backlight brightness (0..100 %)
 */
void circular_clock_set_brightness(uint8_t brightness_pct);

/**
 * @brief Get current brightness setting
 * @return Brightness percentage (0..100)
 */
uint8_t circular_clock_get_brightness(void);

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

#include "clock_settings.h"

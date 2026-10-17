/**
 * @file main.cpp
 * @brief 24-hour circular clock display demo for ESP32-P4
 *
 * Hardware Configuration:
 *  - Board: ESP32-P4
 *  - Display: GC9A01 240x240 Circular Display (SPI2_HOST)
 *  - Pins: MOSI=28, SCLK=27, CS=24, DC=25, RST=26, BKL=-1
 *
 * Demonstrates:
 *  - Dual-ring luxury dial with 24h red comet & 60m cyan comet
 *  - Minute divider track with pulsing amber second pip
 *  - Time status: UNKNOWN -> SEARCHING -> SYNCHRONIZED -> HOLDOVER
 *  - Smart Auto Badge at CY-36 (clean "24", antenna during sync)
 *  - German / English DIN date format (e.g. "Fr, 16. Okt")
 *  - WiFi & SNTP Internet Time Synchronization (pool.ntp.org, time.google.com, ptbtime1.ptb.de)
 *  - Automatic daylight saving (Sommerzeit/Winterzeit) via CET/CEST POSIX timezone
 *  - Approximate date-based moon phase
 */

#include <stdio.h>
#include <atomic>
#include <time.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "circular_clock.h"
#include "wifi_sntp_manager.h"

static const char* TAG = "clock_demo";

#ifndef CONFIG_CLOCK_WIFI_SSID
#define CONFIG_CLOCK_WIFI_SSID "YOUR_WIFI_SSID"
#endif

#ifndef CONFIG_CLOCK_WIFI_PASSWORD
#define CONFIG_CLOCK_WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
#endif

#ifndef CONFIG_CLOCK_TIMEZONE
#define CONFIG_CLOCK_TIMEZONE "CET-1CEST,M3.5.0,M10.5.0/3"
#endif

static std::atomic<bool> s_sync_notification_pending{false};

static void on_ntp_sync_completed(void) {
    s_sync_notification_pending.store(true);
}

static bool is_plausible_system_time(time_t timestamp) {
    constexpr time_t earliest_supported = 1577836800;  // 2020-01-01 UTC
    constexpr time_t latest_supported = 4102444799;    // 2099-12-31 UTC
    return timestamp >= earliest_supported && timestamp <= latest_supported;
}

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "========================================================");
    ESP_LOGI(TAG, "   24-HOUR CIRCULAR CLOCK DISPLAY DEMO                 ");
    ESP_LOGI(TAG, "   Target: %s | Display: GC9A01 240x240 SPI            ", CONFIG_IDF_TARGET);
    ESP_LOGI(TAG, "========================================================");

    // ========================================================================
    // 1. Display Hardware Pin Configuration (from Kconfig — universal)
    // ========================================================================
    circular_clock_config_t config = {
        .spi_host     = SPI2_HOST,
        .pin_mosi     = CONFIG_CIRCULAR_CLOCK_PIN_MOSI,
        .pin_sclk     = CONFIG_CIRCULAR_CLOCK_PIN_SCLK,
        .pin_cs       = CONFIG_CIRCULAR_CLOCK_PIN_CS,
        .pin_dc       = CONFIG_CIRCULAR_CLOCK_PIN_DC,
        .pin_rst      = CONFIG_CIRCULAR_CLOCK_PIN_RST,
        .pin_bkl      = CONFIG_CIRCULAR_CLOCK_PIN_BKL,
        .spi_clock_hz = CONFIG_CIRCULAR_CLOCK_SPI_CLOCK_HZ,
    };

    ESP_LOGI(TAG, "Configuring SPI pins: MOSI=%d, SCLK=%d, CS=%d, DC=%d, RST=%d, BKL=%d",
             config.pin_mosi, config.pin_sclk, config.pin_cs, config.pin_dc, config.pin_rst, config.pin_bkl);

    esp_err_t ret = circular_clock_init(&config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "[ERROR] circular_clock_init failed: %s", esp_err_to_name(ret));
        return;
    }
    ESP_LOGI(TAG, "[OK] Circular clock display initialized successfully");

    // ========================================================================
    // 2. Watchmaker Preferences (from Kconfig — configurable via menuconfig)
    // ========================================================================
#if defined(CONFIG_CLOCK_LANG_CHOICE_EN)
    const char* date_locale = "en-GB";
#elif defined(CONFIG_CLOCK_LANG_CHOICE_EN_US)
    const char* date_locale = "en-US";
#elif defined(CONFIG_CLOCK_LANG_CHOICE_FR)
    const char* date_locale = "fr-FR";
#elif defined(CONFIG_CLOCK_LANG_CHOICE_ES)
    const char* date_locale = "es-ES";
#elif defined(CONFIG_CLOCK_LANG_CHOICE_PT)
    const char* date_locale = "pt-PT";
#elif defined(CONFIG_CLOCK_LANG_CHOICE_IT)
    const char* date_locale = "it-IT";
#elif defined(CONFIG_CLOCK_LANG_CHOICE_DA)
    const char* date_locale = "da-DK";
#elif defined(CONFIG_CLOCK_LANG_CHOICE_NB)
    const char* date_locale = "nb-NO";
#elif defined(CONFIG_CLOCK_LANG_CHOICE_SV)
    const char* date_locale = "sv-SE";
#else
    const char* date_locale = "de-DE";
#endif
    ret = circular_clock_set_locale(date_locale);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Unsupported configured date locale %s: %s",
                 date_locale, esp_err_to_name(ret));
        return;
    }

#if defined(CONFIG_CLOCK_DATE_FORMAT_NUMERIC)
    circular_clock_set_date_format(CLOCK_DATE_FMT_NUMERIC);
#elif defined(CONFIG_CLOCK_DATE_FORMAT_FULL_WEEKDAY)
    circular_clock_set_date_format(CLOCK_DATE_FMT_FULL_WEEKDAY);
#else
    circular_clock_set_date_format(CLOCK_DATE_FMT_ABBREV);
#endif

#if defined(CONFIG_CLOCK_BADGE_CHOICE_ALWAYS_ANTENNA)
    circular_clock_set_badge_mode(CLOCK_BADGE_ALWAYS_ANTENNA);
#elif defined(CONFIG_CLOCK_BADGE_CHOICE_ALWAYS_MODE)
    circular_clock_set_badge_mode(CLOCK_BADGE_ALWAYS_MODE);
#else
    circular_clock_set_badge_mode(CLOCK_BADGE_SMART_AUTO);
#endif

    // Set POSIX timezone for automatic daylight saving (Sommerzeit/Winterzeit)
    setenv("TZ", CONFIG_CLOCK_TIMEZONE, 1);
    tzset();

    // ========================================================================
    // 3. Start time-source acquisition
    // ========================================================================
    circular_clock_set_time_valid(false);
    circular_clock_set_radio_status(CIRCULAR_RADIO_UNKNOWN);

    bool time_acquisition_started = false;
#if defined(CONFIG_CLOCK_WIFI_ENABLE) && CONFIG_CLOCK_WIFI_ENABLE
    ESP_LOGI(TAG, "Starting Wi-Fi/SNTP time acquisition");
    esp_err_t wifi_ret = wifi_sntp_manager_start(CONFIG_CLOCK_WIFI_SSID,
                                                  CONFIG_CLOCK_WIFI_PASSWORD,
                                                  on_ntp_sync_completed);
    if (wifi_ret == ESP_OK) {
        time_acquisition_started = true;
        circular_clock_set_radio_status(CIRCULAR_RADIO_SEARCHING);
    } else {
        ESP_LOGW(TAG, "Time acquisition unavailable: %s",
                 esp_err_to_name(wifi_ret));
    }
#else
    ESP_LOGI(TAG, "Wi-Fi/SNTP is disabled; time will remain unknown");
#endif

    uint32_t frame_count = 0;
    TickType_t last_log = xTaskGetTickCount();
    int last_year = 0;
    int last_month = 0;
    int last_day = 0;

    // ========================================================================
    // 5. 30 FPS Render Loop
    // ========================================================================
    while (true) {
        const bool sync_notification = s_sync_notification_pending.exchange(false);

        struct timeval tv_cur;
        const bool have_system_time = gettimeofday(&tv_cur, NULL) == 0;
        struct tm cur_tm;
        const bool time_valid = have_system_time &&
                                is_plausible_system_time(tv_cur.tv_sec) &&
                                localtime_r(&tv_cur.tv_sec, &cur_tm) != NULL;
        const bool time_source_confirmed = wifi_sntp_manager_is_time_synced();
        const bool accepted_time = time_valid && time_source_confirmed;

        if (sync_notification) {
            if (accepted_time) {
                ESP_LOGI(TAG, "System time synchronized by SNTP");
            } else {
                ESP_LOGE(TAG, "SNTP reported sync, but system time failed validation");
            }
        }

        if (accepted_time) {
            circular_clock_set_time(cur_tm.tm_hour, cur_tm.tm_min, cur_tm.tm_sec,
                                    (int)(tv_cur.tv_usec / 1000));
            circular_clock_set_time_valid(true);
            const circular_radio_status_t status = circular_clock_get_radio_status();
            if (!wifi_sntp_manager_is_connected()) {
                circular_clock_set_radio_status(CIRCULAR_RADIO_FREE_RUN);
            } else if (sync_notification) {
                circular_clock_set_radio_status(CIRCULAR_RADIO_LOCKED);
            } else if (status == CIRCULAR_RADIO_UNKNOWN) {
                circular_clock_set_radio_status(CIRCULAR_RADIO_SEARCHING);
            }
            const int year = cur_tm.tm_year + 1900;
            const int month = cur_tm.tm_mon + 1;
            if (year != last_year || month != last_month || cur_tm.tm_mday != last_day) {
                circular_clock_set_date(year, month, cur_tm.tm_mday, cur_tm.tm_wday);
                last_year = year;
                last_month = month;
                last_day = cur_tm.tm_mday;
            }
        } else {
            circular_clock_set_time_valid(false);
            if (wifi_sntp_manager_is_time_synced() || !time_acquisition_started) {
                circular_clock_set_radio_status(CIRCULAR_RADIO_UNKNOWN);
            } else {
                circular_clock_set_radio_status(CIRCULAR_RADIO_SEARCHING);
            }
        }

        // Render frame
        circular_clock_render();

        frame_count++;

        // Status log every 5 seconds
        TickType_t now_ticks = xTaskGetTickCount();
        if ((now_ticks - last_log) >= pdMS_TO_TICKS(5000)) {
            circular_radio_status_t st = circular_clock_get_radio_status();
            const char* st_name = (st == CIRCULAR_RADIO_SEARCHING) ? "SEARCHING" :
                                  (st == CIRCULAR_RADIO_LOCKED)    ? "SYNCHRONIZED" :
                                  (st == CIRCULAR_RADIO_FREE_RUN)  ? "HOLDOVER" : "UNKNOWN";
            ESP_LOGI(TAG, "[%s] Status: %s | Calibrating: %s | WiFi: %s | Frames: %lu",
                     accepted_time ? "time valid" : "time unknown",
                     st_name, circular_clock_is_calibrating() ? "YES" : "NO",
                     wifi_sntp_manager_is_connected() ? "CONNECTED" : "CONNECTING/OFFLINE",
                     frame_count);
            last_log = now_ticks;
        }

        // 30 FPS (~33ms per frame)
        vTaskDelay(pdMS_TO_TICKS(33));
    }
}

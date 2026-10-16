/**
 * @file main.cpp
 * @brief Luxury 24h Circular Radio Clock (Funkuhr) Demo for ESP32-P4 on COM4
 *
 * Hardware Configuration:
 *  - Board: ESP32-P4
 *  - Display: GC9A01 240x240 Circular Display (SPI2_HOST)
 *  - Pins: MOSI=28, SCLK=27, CS=24, DC=25, RST=26, BKL=-1
 *
 * Demonstrates:
 *  - Dual-ring luxury dial with 24h red comet & 60m cyan comet
 *  - High-precision Trennschiene divider track with pulsing amber second pip
 *  - Funkuhr Status: SEARCHING -> LOCKED -> FREE_RUN
 *  - Smart Auto Badge at CY-36 (clean "24", antenna during sync)
 *  - German / English DIN date format (e.g. "Fr, 16. Okt")
 *  - WiFi & SNTP Internet Time Synchronization (pool.ntp.org, time.google.com, ptbtime1.ptb.de)
 *  - Automatic daylight saving (Sommerzeit/Winterzeit) via CET/CEST POSIX timezone
 *  - Real-time astronomical Moon Phase from Julian Day calculation
 */

#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
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

static bool s_atomic_sync_done = false;

static void on_ntp_sync_completed(void) {
    ESP_LOGI(TAG, "========================================================");
    ESP_LOGI(TAG, "  ATOMIC CLOCK SYNCHRONIZATION CONFIRMED VIA INTERNET   ");
    ESP_LOGI(TAG, "========================================================");
    s_atomic_sync_done = true;
    circular_clock_set_radio_status(CIRCULAR_RADIO_LOCKED);
}

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "========================================================");
    ESP_LOGI(TAG, "   24h LUXUS-FUNKUHR (CIRCULAR ATOMIC CLOCK DEMO)      ");
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
    circular_clock_set_language(CLOCK_LANG_EN);
#else
    circular_clock_set_language(CLOCK_LANG_DE);
#endif

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

    // Baseline offline time (will be overwritten once NTP locks)
    time_t t_now = time(NULL);
    struct tm cur_boot;
    localtime_r(&t_now, &cur_boot);

    circular_clock_set_date(cur_boot.tm_year + 1900, cur_boot.tm_mon + 1, cur_boot.tm_mday, cur_boot.tm_wday);

    // ========================================================================
    // 3. Funkuhr Signal Acquisition Start
    // ========================================================================
    ESP_LOGI(TAG, "Starting Funkuhr in SEARCHING mode (Radar sweep active)");
    circular_clock_set_radio_status(CIRCULAR_RADIO_SEARCHING);

    // ========================================================================
    // 4. WiFi & SNTP Connection (with provided credentials)
    // ========================================================================
    ESP_LOGI(TAG, "Connecting to WiFi SSID: '%s'...", CONFIG_CLOCK_WIFI_SSID);
    esp_err_t wifi_ret = wifi_sntp_manager_start(CONFIG_CLOCK_WIFI_SSID,
                                                 CONFIG_CLOCK_WIFI_PASSWORD,
                                                 on_ntp_sync_completed);
    if (wifi_ret != ESP_OK) {
        ESP_LOGW(TAG, "WiFi start returned %s. Proceeding in autonomous quartz demo mode.", esp_err_to_name(wifi_ret));
    }

    uint32_t boot_ms = (uint32_t)(esp_timer_get_time() / 1000);
    bool simulated_lock_done = false;
    uint32_t frame_count = 0;
    TickType_t last_log = xTaskGetTickCount();

    // ========================================================================
    // 5. High-Precision 30 FPS Render Loop
    // ========================================================================
    while (true) {
        uint32_t current_ms = (uint32_t)(esp_timer_get_time() / 1000);
        uint32_t elapsed_ms = current_ms - boot_ms;

        // Fallback demo synchronization if offline or waiting for WiFi
        if (!s_atomic_sync_done && !simulated_lock_done && elapsed_ms >= 6000) {
            ESP_LOGI(TAG, ">>> SIGNAL ACQUIRED! Calibrating atomic lock <<<");
            circular_clock_set_radio_status(CIRCULAR_RADIO_LOCKED);
            simulated_lock_done = true;
        }

        // Get current system time (synchronized via NTP or local oscillator)
        struct timeval tv_cur;
        gettimeofday(&tv_cur, NULL);
        struct tm cur_tm;
        localtime_r(&tv_cur.tv_sec, &cur_tm);
        int sub_ms = (int)(tv_cur.tv_usec / 1000);

        // Update clock model
        circular_clock_set_time(cur_tm.tm_hour, cur_tm.tm_min, cur_tm.tm_sec, sub_ms);
        circular_clock_set_date(cur_tm.tm_year + 1900, cur_tm.tm_mon + 1, cur_tm.tm_mday, cur_tm.tm_wday);

        // Render frame
        circular_clock_render();

        frame_count++;

        // Status log every 5 seconds
        TickType_t now_ticks = xTaskGetTickCount();
        if ((now_ticks - last_log) >= pdMS_TO_TICKS(5000)) {
            circular_radio_status_t st = circular_clock_get_radio_status();
            const char* st_name = (st == CIRCULAR_RADIO_SEARCHING) ? "SEARCHING (Radar Sweep)" :
                                  (st == CIRCULAR_RADIO_LOCKED)    ? "LOCKED (Atomic Time)" : "FREE_RUN (Quartz)";
            ESP_LOGI(TAG, "[%02d:%02d:%02d] Radio: %s | Calibrating: %s | WiFi: %s | Frames: %lu",
                     cur_tm.tm_hour, cur_tm.tm_min, cur_tm.tm_sec,
                     st_name, circular_clock_is_calibrating() ? "YES" : "NO",
                     wifi_sntp_manager_is_connected() ? "CONNECTED" : "CONNECTING/OFFLINE",
                     frame_count);
            last_log = now_ticks;
        }

        // 30 FPS (~33ms per frame)
        vTaskDelay(pdMS_TO_TICKS(33));
    }
}

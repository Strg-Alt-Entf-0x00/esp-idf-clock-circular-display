/**
 * @file display_hal.cpp
 * @brief GC9A01 240x240 circular SPI panel driver and LovyanGFX sprite buffer for ESP32-P4
 */

#include "circular_clock.h"
#include "esp_log.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_gc9a01.h"
#include "driver/ledc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define LGFX_USE_V1
#include <LovyanGFX.hpp>

static const char* TAG = "display_hal";

#define LCD_PIXEL_CLOCK_HZ (40 * 1000 * 1000)
#define LCD_H_RES 240
#define LCD_V_RES 240

namespace {
class LGFX_Clock_Dummy : public lgfx::LGFX_Device {
public:
    LGFX_Clock_Dummy(void) {}
};
}
static LGFX_Clock_Dummy s_lcd;
static lgfx::LGFX_Sprite s_sprite(&s_lcd);
static esp_lcd_panel_handle_t s_panel_handle = NULL;
static int s_backlight_pin = -1;

esp_err_t circular_clock_hal_init(const circular_clock_config_t* config) {
    if (!config) {
        ESP_LOGE(TAG, "Config is NULL");
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Initializing GC9A01 240x240 circular display");

    // 1. SPI Bus Configuration
    spi_bus_config_t buscfg = {};
    buscfg.mosi_io_num = config->pin_mosi;
    buscfg.miso_io_num = -1;
    buscfg.sclk_io_num = config->pin_sclk;
    buscfg.quadwp_io_num = -1;
    buscfg.quadhd_io_num = -1;
    buscfg.max_transfer_sz = LCD_H_RES * LCD_V_RES * sizeof(uint16_t);

    esp_err_t ret = spi_bus_initialize(config->spi_host, &buscfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "spi_bus_initialize failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // 2. Install Panel IO
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_config = {};
    io_config.cs_gpio_num = (gpio_num_t)config->pin_cs;
    io_config.dc_gpio_num = (gpio_num_t)config->pin_dc;
    io_config.spi_mode = 0;
    io_config.pclk_hz = config->spi_clock_hz > 0 ? config->spi_clock_hz : LCD_PIXEL_CLOCK_HZ;
    io_config.trans_queue_depth = 10;
    io_config.lcd_cmd_bits = 8;
    io_config.lcd_param_bits = 8;

    ret = esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)config->spi_host, &io_config, &io_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_lcd_new_panel_io_spi failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // 3. Install GC9A01 Panel Driver
    esp_lcd_panel_dev_config_t panel_config = {};
    panel_config.rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR;
    panel_config.bits_per_pixel = 16;
    panel_config.reset_gpio_num = (gpio_num_t)config->pin_rst;

    ret = esp_lcd_new_panel_gc9a01(io_handle, &panel_config, &s_panel_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_lcd_new_panel_gc9a01 failed: %s", esp_err_to_name(ret));
        return ret;
    }

    esp_lcd_panel_reset(s_panel_handle);
    esp_lcd_panel_init(s_panel_handle);
    esp_lcd_panel_invert_color(s_panel_handle, true);
    esp_lcd_panel_mirror(s_panel_handle, true, false);
    esp_lcd_panel_disp_on_off(s_panel_handle, true);

    // 4. Initialize LovyanGFX sprite buffer (uses PSRAM on ESP32-P4)
    s_lcd.init();
    s_lcd.setRotation(0);
    s_lcd.setBrightness(255);

    s_sprite.setColorDepth(16);
    s_sprite.setPsram(true);
    s_sprite.createSprite(LCD_H_RES, LCD_V_RES);
    if (!s_sprite.getBuffer()) {
        ESP_LOGE(TAG, "Failed to allocate 240x240 sprite in PSRAM");
        return ESP_ERR_NO_MEM;
    }
    s_sprite.setTextDatum(lgfx::middle_center);
    s_sprite.setSwapBytes(true);

    // 5. Backlight (LEDC PWM)
    s_backlight_pin = config->pin_bkl;
    if (s_backlight_pin >= 0) {
        ledc_timer_config_t ledc_timer = {};
        ledc_timer.speed_mode       = LEDC_LOW_SPEED_MODE;
        ledc_timer.timer_num        = LEDC_TIMER_0;
        ledc_timer.duty_resolution  = LEDC_TIMER_8_BIT;
        ledc_timer.freq_hz          = 5000;
        ledc_timer.clk_cfg          = LEDC_AUTO_CLK;
        ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

        ledc_channel_config_t ledc_channel = {};
        ledc_channel.speed_mode     = LEDC_LOW_SPEED_MODE;
        ledc_channel.channel        = LEDC_CHANNEL_0;
        ledc_channel.timer_sel      = LEDC_TIMER_0;
        ledc_channel.gpio_num       = s_backlight_pin;
        ledc_channel.duty           = 0;
        ledc_channel.hpoint         = 0;
        ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

        // Soft fade-in
        for (int i = 0; i <= 255; i += 5) {
            ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, i);
            ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }

    ESP_LOGI(TAG, "Display HAL ready (240x240 GC9A01 + PSRAM Sprite)");
    return ESP_OK;
}

void display_hal_set_brightness(uint8_t brightness_pct) {
    if (s_backlight_pin < 0) return;
    if (brightness_pct > 100) brightness_pct = 100;
    uint32_t duty = (255 * brightness_pct) / 100;
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}

void* circular_clock_get_sprite(void) {
    return &s_sprite;
}

void circular_clock_push_frame(void) {
    if (s_panel_handle != NULL && s_sprite.getBuffer() != NULL) {
        esp_lcd_panel_draw_bitmap(s_panel_handle, 0, 0, LCD_H_RES, LCD_V_RES, s_sprite.getBuffer());
    }
}

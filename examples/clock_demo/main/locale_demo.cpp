/**
 * @file locale_demo.cpp
 * @brief Automatic demo cycling through all 10 supported locales
 */

#include "locale_demo.h"
#include "circular_clock.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char* TAG = "locale_demo";

void locale_demo_cycle_all(void) {
    uint8_t locale_count = circular_clock_get_locale_count();
    ESP_LOGI(TAG, "======================================");
    ESP_LOGI(TAG, "  LOCALE DEMO: Cycling through %d languages", locale_count);
    ESP_LOGI(TAG, "  Each language shown for 10 seconds");
    ESP_LOGI(TAG, "======================================");
    
    for (uint8_t i = 0; i < locale_count; i++) {
        char tag[16];
        esp_err_t err = circular_clock_get_locale_tag(i, tag, sizeof(tag));
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to get locale %d", i);
            continue;
        }
        
        ESP_LOGI(TAG, "[%d/%d] Switching to: %s", i+1, locale_count, tag);
        
        err = circular_clock_set_locale(tag);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to set locale %s: %s", tag, esp_err_to_name(err));
            continue;
        }
        
        // Show locale for 10 seconds
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
    
    ESP_LOGI(TAG, "Locale demo cycle complete!");
}

void locale_demo_print_all(void) {
    uint8_t locale_count = circular_clock_get_locale_count();
    
    ESP_LOGI(TAG, "======================================");
    ESP_LOGI(TAG, "  Available Locales: %d", locale_count);
    ESP_LOGI(TAG, "======================================");
    
    for (uint8_t i = 0; i < locale_count; i++) {
        char tag[16];
        if (circular_clock_get_locale_tag(i, tag, sizeof(tag)) == ESP_OK) {
            ESP_LOGI(TAG, "  [%d] %s", i, tag);
        }
    }
    
    char current[16];
    if (circular_clock_get_locale(current, sizeof(current)) == ESP_OK) {
        ESP_LOGI(TAG, "Current: %s", current);
    }
    ESP_LOGI(TAG, "======================================");
}

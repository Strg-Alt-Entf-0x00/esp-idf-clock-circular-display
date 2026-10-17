/**
 * @file locale_demo.h
 * @brief Locale demonstration functions
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Cycle through all supported locales automatically
 * 
 * Displays each locale for 10 seconds, showing:
 * - Date formatting
 * - Weekday names
 * - Month names
 * 
 * Useful for hardware testing and visual verification.
 */
void locale_demo_cycle_all(void);

/**
 * @brief Print all available locales to console
 */
void locale_demo_print_all(void);

#ifdef __cplusplus
}
#endif

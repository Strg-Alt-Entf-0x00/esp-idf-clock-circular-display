/**
 * @file radio_sync.h
 * @brief Radio synchronization status, source types, and event callbacks
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Funkuhr operation status
 */
typedef enum {
    CIRCULAR_RADIO_FREE_RUN = 0,    /**< Holdover using the local clock without an active source */
    CIRCULAR_RADIO_SEARCHING = 1,   /**< A time source is being acquired (radar sweep active) */
    CIRCULAR_RADIO_LOCKED = 2,      /**< Time was confirmed by a sync event; not active reception */
    CIRCULAR_RADIO_UNKNOWN = 3      /**< No valid time source has been established */
} circular_radio_status_t;

/**
 * @brief Supported time synchronization sources
 */
typedef enum {
    CIRCULAR_RADIO_SRC_NTP = 0,     /**< Network time via NTP/SNTP */
    CIRCULAR_RADIO_SRC_DCF77 = 1,   /**< DCF77 77.5 kHz longwave receiver */
    CIRCULAR_RADIO_SRC_GPS = 2,     /**< GPS / GNSS NMEA receiver */
    CIRCULAR_RADIO_SRC_SIMULATED = 3/**< Simulated synchronization for demo/testing */
} circular_radio_source_t;

/**
 * @brief Radio event callback signature
 */
typedef void (*circular_radio_event_cb_t)(circular_radio_status_t status, void* user_data);

#ifdef __cplusplus
}
#endif

/**
 * @file locale_data.h
 * @brief Locale definitions for clock date formatting
 * 
 * Supported locales with Latin-1 font coverage (U+0020–U+007E, U+00A0–U+00FF):
 * - German (de-DE)
 * - English UK/US (en-GB, en-US)
 * - French (fr-FR)
 * - Spanish (es-ES)
 * - Portuguese (pt-PT)
 * - Italian (it-IT)
 * - Danish (da-DK)
 * - Norwegian Bokmål (nb-NO)
 * - Swedish (sv-SE)
 */

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Date component ordering
 */
typedef enum {
    LOCALE_DATE_ORDER_DAY_MONTH = 0,  /**< Day before month (European style) */
    LOCALE_DATE_ORDER_MONTH_DAY = 1   /**< Month before day (US style) */
} locale_date_order_t;

/**
 * @brief Locale definition structure
 */
typedef struct {
    const char* tag;                        /**< BCP 47 tag (lowercase, e.g. "de-de") */
    const char* const* weekdays_short;      /**< 7 short weekday names [Sun..Sat] */
    const char* const* weekdays_full;       /**< 7 full weekday names [Sunday..Saturday] */
    const char* const* months_short;        /**< 12 abbreviated month names [Jan..Dec] */
    locale_date_order_t date_order;         /**< Date component ordering */
    const char* weekday_separator;          /**< Separator after weekday (e.g. ", " or " ") */
    const char* day_month_separator;        /**< Separator between day and month */
    char numeric_separator;                 /**< Numeric date separator ('.' or '/') */
    bool numeric_month_first;               /**< True for MM/DD, false for DD.MM */
} locale_definition_t;

/**
 * @brief Get number of supported locales
 */
uint8_t locale_data_get_count(void);

/**
 * @brief Get locale definition by index
 * @param index Index [0..count-1]
 * @return Pointer to locale definition or NULL if out of range
 */
const locale_definition_t* locale_data_get_by_index(uint8_t index);

/**
 * @brief Find locale by BCP 47 tag (case-insensitive)
 * @param tag BCP 47 tag (e.g. "de-DE", "en", "FR-fr")
 * @return Pointer to locale definition or NULL if not found
 */
const locale_definition_t* locale_data_find(const char* tag);

/**
 * @brief Validate locale tag format
 * @param tag Tag to validate
 * @return true if format is valid, false otherwise
 */
bool locale_data_is_valid_tag(const char* tag);

#ifdef __cplusplus
}
#endif

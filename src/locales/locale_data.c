/**
 * @file locale_data.c
 * @brief Locale database implementation
 */

#include "locale_data.h"
#include <string.h>
#include <ctype.h>

// ============================================================================
// German (Germany)
// ============================================================================
static const char* const s_wdays_de_short[] = {
    "So", "Mo", "Di", "Mi", "Do", "Fr", "Sa"
};
static const char* const s_wdays_de_full[] = {
    "Sonntag", "Montag", "Dienstag", "Mittwoch", "Donnerstag", "Freitag", "Samstag"
};
static const char* const s_months_de[] = {
    "Jan", "Feb", "Mrz", "Apr", "Mai", "Jun", 
    "Jul", "Aug", "Sep", "Okt", "Nov", "Dez"
};

// ============================================================================
// English (United Kingdom, United States)
// ============================================================================
static const char* const s_wdays_en_short[] = {
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};
static const char* const s_wdays_en_full[] = {
    "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"
};
static const char* const s_months_en[] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun", 
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

// ============================================================================
// French (France)
// ============================================================================
static const char* const s_wdays_fr_short[] = {
    "dim.", "lun.", "mar.", "mer.", "jeu.", "ven.", "sam."
};
static const char* const s_wdays_fr_full[] = {
    "dimanche", "lundi", "mardi", "mercredi", "jeudi", "vendredi", "samedi"
};
static const char* const s_months_fr[] = {
    "janv.", "févr.", "mars", "avr.", "mai", "juin", 
    "juil.", "août", "sept.", "oct.", "nov.", "déc."
};

// ============================================================================
// Spanish (Spain)
// ============================================================================
static const char* const s_wdays_es_short[] = {
    "dom.", "lun.", "mar.", "mié.", "jue.", "vie.", "sáb."
};
static const char* const s_wdays_es_full[] = {
    "domingo", "lunes", "martes", "miércoles", "jueves", "viernes", "sábado"
};
static const char* const s_months_es[] = {
    "ene.", "feb.", "mar.", "abr.", "may.", "jun.", 
    "jul.", "ago.", "sept.", "oct.", "nov", "dic."
};

// ============================================================================
// Portuguese (Portugal)
// ============================================================================
static const char* const s_wdays_pt_short[] = {
    "dom.", "seg.", "ter.", "qua.", "qui.", "sex.", "sáb."
};
static const char* const s_wdays_pt_full[] = {
    "domingo", "segunda-feira", "terça-feira", "quarta-feira", 
    "quinta-feira", "sexta-feira", "sábado"
};
static const char* const s_months_pt[] = {
    "jan.", "fev.", "mar.", "abr.", "mai.", "jun.", 
    "jul.", "ago.", "set.", "out.", "nov.", "dez."
};

// ============================================================================
// Italian (Italy)
// ============================================================================
static const char* const s_wdays_it_short[] = {
    "dom", "lun", "mar", "mer", "gio", "ven", "sab"
};
static const char* const s_wdays_it_full[] = {
    "domenica", "lunedì", "martedì", "mercoledì", "giovedì", "venerdì", "sabato"
};
static const char* const s_months_it[] = {
    "gen", "feb", "mar", "apr", "mag", "giu", 
    "lug", "ago", "set", "ott", "nov", "dic"
};

// ============================================================================
// Danish (Denmark)
// ============================================================================
static const char* const s_wdays_da_short[] = {
    "søn.", "man.", "tir.", "ons.", "tor.", "fre.", "lør."
};
static const char* const s_wdays_da_full[] = {
    "søndag", "mandag", "tirsdag", "onsdag", "torsdag", "fredag", "lørdag"
};
static const char* const s_months_da[] = {
    "jan.", "feb.", "mar.", "apr.", "maj", "jun.", 
    "jul.", "aug.", "sep.", "okt.", "nov.", "dec."
};

// ============================================================================
// Norwegian Bokmål (Norway)
// ============================================================================
static const char* const s_wdays_nb_short[] = {
    "søn.", "man.", "tir.", "ons.", "tor.", "fre.", "lør."
};
static const char* const s_wdays_nb_full[] = {
    "søndag", "mandag", "tirsdag", "onsdag", "torsdag", "fredag", "lørdag"
};
static const char* const s_months_nb[] = {
    "jan.", "feb.", "mars", "apr.", "mai", "juni", 
    "juli", "aug.", "sep.", "okt.", "nov.", "des."
};

// ============================================================================
// Swedish (Sweden)
// ============================================================================
static const char* const s_wdays_sv_short[] = {
    "sön", "mån", "tis", "ons", "tor", "fre", "lör"
};
static const char* const s_wdays_sv_full[] = {
    "söndag", "måndag", "tisdag", "onsdag", "torsdag", "fredag", "lördag"
};
static const char* const s_months_sv[] = {
    "jan.", "feb.", "mars", "apr.", "maj", "juni", 
    "juli", "aug.", "sep.", "okt.", "nov.", "dec."
};

// ============================================================================
// Locale Registry
// ============================================================================
static const locale_definition_t s_locales[] = {
    {
        .tag = "de-de",
        .weekdays_short = s_wdays_de_short,
        .weekdays_full = s_wdays_de_full,
        .months_short = s_months_de,
        .date_order = LOCALE_DATE_ORDER_DAY_MONTH,
        .weekday_separator = ", ",
        .day_month_separator = ". ",
        .numeric_separator = '.',
        .numeric_month_first = false
    },
    {
        .tag = "en-gb",
        .weekdays_short = s_wdays_en_short,
        .weekdays_full = s_wdays_en_full,
        .months_short = s_months_en,
        .date_order = LOCALE_DATE_ORDER_DAY_MONTH,
        .weekday_separator = ", ",
        .day_month_separator = " ",
        .numeric_separator = '.',
        .numeric_month_first = false
    },
    {
        .tag = "en-us",
        .weekdays_short = s_wdays_en_short,
        .weekdays_full = s_wdays_en_full,
        .months_short = s_months_en,
        .date_order = LOCALE_DATE_ORDER_MONTH_DAY,
        .weekday_separator = ", ",
        .day_month_separator = " ",
        .numeric_separator = '/',
        .numeric_month_first = true
    },
    {
        .tag = "fr-fr",
        .weekdays_short = s_wdays_fr_short,
        .weekdays_full = s_wdays_fr_full,
        .months_short = s_months_fr,
        .date_order = LOCALE_DATE_ORDER_DAY_MONTH,
        .weekday_separator = " ",
        .day_month_separator = " ",
        .numeric_separator = '/',
        .numeric_month_first = false
    },
    {
        .tag = "es-es",
        .weekdays_short = s_wdays_es_short,
        .weekdays_full = s_wdays_es_full,
        .months_short = s_months_es,
        .date_order = LOCALE_DATE_ORDER_DAY_MONTH,
        .weekday_separator = ", ",
        .day_month_separator = " ",
        .numeric_separator = '/',
        .numeric_month_first = false
    },
    {
        .tag = "pt-pt",
        .weekdays_short = s_wdays_pt_short,
        .weekdays_full = s_wdays_pt_full,
        .months_short = s_months_pt,
        .date_order = LOCALE_DATE_ORDER_DAY_MONTH,
        .weekday_separator = ", ",
        .day_month_separator = " de ",
        .numeric_separator = '/',
        .numeric_month_first = false
    },
    {
        .tag = "it-it",
        .weekdays_short = s_wdays_it_short,
        .weekdays_full = s_wdays_it_full,
        .months_short = s_months_it,
        .date_order = LOCALE_DATE_ORDER_DAY_MONTH,
        .weekday_separator = " ",
        .day_month_separator = " ",
        .numeric_separator = '/',
        .numeric_month_first = false
    },
    {
        .tag = "da-dk",
        .weekdays_short = s_wdays_da_short,
        .weekdays_full = s_wdays_da_full,
        .months_short = s_months_da,
        .date_order = LOCALE_DATE_ORDER_DAY_MONTH,
        .weekday_separator = " ",
        .day_month_separator = ". ",
        .numeric_separator = '/',
        .numeric_month_first = false
    },
    {
        .tag = "nb-no",
        .weekdays_short = s_wdays_nb_short,
        .weekdays_full = s_wdays_nb_full,
        .months_short = s_months_nb,
        .date_order = LOCALE_DATE_ORDER_DAY_MONTH,
        .weekday_separator = " ",
        .day_month_separator = ". ",
        .numeric_separator = '/',
        .numeric_month_first = false
    },
    {
        .tag = "sv-se",
        .weekdays_short = s_wdays_sv_short,
        .weekdays_full = s_wdays_sv_full,
        .months_short = s_months_sv,
        .date_order = LOCALE_DATE_ORDER_DAY_MONTH,
        .weekday_separator = " ",
        .day_month_separator = " ",
        .numeric_separator = '/',
        .numeric_month_first = false
    }
};

#define LOCALE_COUNT (sizeof(s_locales) / sizeof(s_locales[0]))

// ============================================================================
// Public API Implementation
// ============================================================================

uint8_t locale_data_get_count(void) {
    return LOCALE_COUNT;
}

const locale_definition_t* locale_data_get_by_index(uint8_t index) {
    if (index >= LOCALE_COUNT) {
        return NULL;
    }
    return &s_locales[index];
}

bool locale_data_is_valid_tag(const char* tag) {
    if (!tag || !tag[0]) {
        return false;
    }
    
    size_t length = 0;
    for (; tag[length] != '\0'; ++length) {
        if (length >= 15) {  // Max reasonable length
            return false;
        }
        const char ch = tag[length];
        if (!((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') ||
              (ch >= '0' && ch <= '9') || ch == '-')) {
            return false;
        }
    }
    
    return length >= 2;  // At least language code
}

const locale_definition_t* locale_data_find(const char* tag) {
    if (!locale_data_is_valid_tag(tag)) {
        return NULL;
    }
    
    // Normalize to lowercase
    char normalized[16];
    size_t length = 0;
    for (; tag[length] != '\0' && length < sizeof(normalized) - 1; ++length) {
        const char ch = tag[length];
        normalized[length] = (ch >= 'A' && ch <= 'Z') 
                                ? (char)(ch - 'A' + 'a') 
                                : ch;
    }
    normalized[length] = '\0';
    
    // Exact match first
    for (uint8_t i = 0; i < LOCALE_COUNT; ++i) {
        if (strcmp(normalized, s_locales[i].tag) == 0) {
            return &s_locales[i];
        }
    }
    
    // Bare language tag fallback (e.g. "de" -> "de-de")
    if (length == 2) {
        for (uint8_t i = 0; i < LOCALE_COUNT; ++i) {
            if (strncmp(normalized, s_locales[i].tag, 2) == 0) {
                return &s_locales[i];
            }
        }
    }
    
    return NULL;
}

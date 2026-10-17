#include "moon_phase.h"

#include <cmath>

namespace {
bool is_valid_date(int year, int month, int day) {
    if (year < 1 || year > 9999 || month < 1 || month > 12) {
        return false;
    }

    static const int days_per_month[] = {31, 28, 31, 30, 31, 30,
                                          31, 31, 30, 31, 30, 31};
    int max_day = days_per_month[month - 1];
    const bool leap_year = (year % 4 == 0) &&
                           ((year % 100 != 0) || (year % 400 == 0));
    if (month == 2 && leap_year) {
        ++max_day;
    }
    return day >= 1 && day <= max_day;
}
}  // namespace

bool clock_moon_phase_for_date(int year, int month, int day, float* phase) {
    if (!phase || !is_valid_date(year, month, day)) {
        return false;
    }

    // Calendar-day approximation using the known 2024-01-11 new moon and the
    // mean synodic month. Exact phase requires a time of day and ephemeris.
    int adjusted_year = year;
    int adjusted_month = month;
    if (adjusted_month < 3) {
        --adjusted_year;
        adjusted_month += 12;
    }
    const int century = adjusted_year / 100;
    const int correction = 2 - century + century / 4;
    const long julian_day = static_cast<long>(365.25 * (adjusted_year + 4716)) +
                            static_cast<long>(30.6001 * (adjusted_month + 1)) +
                            day + correction - 1524;
    const double days_since_new_moon = static_cast<double>(julian_day - 2460321);
    constexpr double mean_synodic_month = 29.53058867;
    double result = std::fmod(days_since_new_moon, mean_synodic_month) /
                    mean_synodic_month;
    if (result < 0.0) {
        result += 1.0;
    }
    *phase = static_cast<float>(result);
    return true;
}

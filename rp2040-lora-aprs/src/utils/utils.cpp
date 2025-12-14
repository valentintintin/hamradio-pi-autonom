#include "utils/utils.h"

#include <ctime>
#include <hardware/rtc.h>

#include "DS3231.h"

void getDateTimeStringFromEpoch(const uint64_t epoch, char* buffer, const size_t size) {
    const time_t epochTimeT = epoch;
    tm ts{};
    localtime_r(&epochTimeT, &ts);
    strftime(buffer, size, "%Y-%m-%dT%H:%M:%S", &ts);
    // snprintf(buffer, size, "%d-%d-%dT%d:%d:%dZ", ts.tm_year + 1900, ts.tm_mon + 1, ts.tm_mday, ts.tm_hour, ts.tm_min, ts.tm_sec);
}
bool isDST(const int16_t year, const int8_t month, const int8_t day, const int8_t hour) {
    if (month < 3 || month > 10) return false;
    if (month > 3 && month < 10) return true;

    const int lastSunday = 31 - ((5 * year / 4 + 4) % 7); // formule pour dernier dimanche

    if (month == 3) {
        // Passage à l'heure d'été
        if (day < lastSunday) return false;
        if (day > lastSunday) return true;
        return hour >= 2;
    } else if (month == 10) {
        // Retour à l'heure d'hiver
        if (day < lastSunday) return true;
        if (day > lastSunday) return false;
        return hour < 3;
    }

    return false;
}

int daysInMonth(const int8_t month, const int16_t year) {
    if (month == 2) return (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)) ? 29 : 28;
    if (month == 4 || month == 6 || month == 9 || month == 11) return 30;
    return 31;
}

void addHours(int16_t &year, int8_t &month, int8_t &day, int8_t &hour, const int8_t hoursToAdd) {
    hour += hoursToAdd;
    while (hour >= 24) {
        hour -= 24;
        day += 1;
        int dim = daysInMonth(month, year);
        if (day > dim) {
            day = 1;
            month += 1;
            if (month > 12) {
                month = 1;
                year += 1;
            }
        }
    }
}

DateTime getDateTime() {
    datetime_t datetime;
    rtc_get_datetime(&datetime);

    int16_t year = datetime.year + 1900;
    int8_t month = datetime.month;
    int8_t day = datetime.day;
    int8_t hour = datetime.hour;

    const auto isDSTNow = isDST(year, month, day, hour);
    addHours(year, month, day, hour, isDSTNow ? 2 : 1);

    return {static_cast<uint16_t>(year), static_cast<uint8_t>(month), static_cast<uint8_t>(day), static_cast<uint8_t>(hour), static_cast<uint8_t>(datetime.min), static_cast<uint8_t>(datetime.sec)};
}
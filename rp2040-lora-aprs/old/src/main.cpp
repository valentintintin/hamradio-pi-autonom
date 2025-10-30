#include "utils.h"
#include "System.h"

char bufferText[BUFFER_LENGTH + 1]{};
uint8_t buffer[BUFFER_LENGTH + 1]{};

System systemControl;

void setup() {
    systemControl.begin();
}

void loop() {
    systemControl.loop();
}

void delayWdt(const uint32_t milliseconds) {
    if (systemControl.settings.useInternalWatchdog) {
        const uint64_t startTime = millis();
        uint64_t elapsedTime = 0;

        while (elapsedTime < milliseconds) {
            if (milliseconds - elapsedTime >= 1000) {
                delay(1000);
                rp2040.wdt_reset();
            } else {
                delay(milliseconds - elapsedTime);
            }
            elapsedTime = millis() - startTime;
        }
    } else {
        delay(milliseconds);
    }
}

void ledBlink(const uint8_t howMany, const uint16_t milliseconds) {
    for (uint8_t i = 0; i < howMany; i++) {
        digitalWrite(PIN_LED, HIGH);
        delayWdt(milliseconds);
        digitalWrite(PIN_LED, LOW);
        delayWdt(milliseconds);
    }
}

void getDateTimeStringFromEpoch(const uint64_t epoch, char* buffer, const size_t size) {
    const time_t epochTimeT = epoch;
    const tm ts = *localtime(&epochTimeT);
    strftime(buffer, size, "%Y-%m-%dT%H:%M:%S", &ts);
    // snprintf(buffer, size, "%d-%d-%dT%d:%d:%dZ", ts.tm_year + 1900, ts.tm_mon + 1, ts.tm_mday, ts.tm_hour, ts.tm_min, ts.tm_sec);
}

float clamp(const float v, const float lo, const float hi) {
    return (v < lo) ? lo : (hi < v) ? hi : v;
}

bool isAfterBoot() {
    return millis() > TIME_AFTER_BOOT;
}

bool isWithinTimespanMs(const uint32_t lastExecutionMs, const uint32_t timeSpanMs) {
    return (millis() - lastExecutionMs) < timeSpanMs;
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
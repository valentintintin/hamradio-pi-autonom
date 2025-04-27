#ifndef RP2040_LORA_APRS_UTILS_H
#define RP2040_LORA_APRS_UTILS_H

#include <Arduino.h>
#include <ArduinoLog.h>

void ledBlink(uint8_t howMany = 3, uint16_t milliseconds = 250);
void delayWdt(uint32_t milliseconds);
void getDateTimeStringFromEpoch(uint64_t epoch, char* buffer, size_t size);
float clamp(float v, float lo, float hi);
bool isAfterBoot();
bool isWithinTimespanMs(uint32_t lastExecutionMs, uint32_t timeSpanMs);
bool isDST(int16_t year, int8_t month, int8_t day, int8_t hour);
int daysInMonth(int8_t month, int16_t year);
void addHours(int16_t &year, int8_t &month, int8_t &day, int8_t &hour, int8_t hoursToAdd);

#endif //RP2040_LORA_APRS_UTILS_H

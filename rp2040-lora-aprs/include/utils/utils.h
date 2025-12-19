#pragma once
#include <Arduino.h>

#include "DS3231.h"

void getDateTimeStringFromEpoch(uint64_t epoch, char* buffer, size_t size);
DateTime getDateTime();
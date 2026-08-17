#pragma once

#include "pico/util/datetime.h"

void rtc_init();
bool rtc_get_datetime(datetime_t* dt);
bool rtc_set_datetime(const datetime_t* dt);

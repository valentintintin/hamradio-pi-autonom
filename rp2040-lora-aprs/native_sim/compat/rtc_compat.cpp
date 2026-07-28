#include "hardware/rtc.h"

#include <ctime>

static long g_offset_seconds = 0;

void rtc_init() {}

bool rtc_get_datetime(datetime_t* dt) {
  time_t now = time(nullptr) + g_offset_seconds;
  struct tm tmv;
  gmtime_r(&now, &tmv);
  dt->year = (int16_t)(tmv.tm_year + 1900);
  dt->month = (int8_t)(tmv.tm_mon + 1);
  dt->day = (int8_t)tmv.tm_mday;
  dt->dotw = (int8_t)tmv.tm_wday;
  dt->hour = (int8_t)tmv.tm_hour;
  dt->min = (int8_t)tmv.tm_min;
  dt->sec = (int8_t)tmv.tm_sec;
  return true;
}

bool rtc_set_datetime(const datetime_t* dt) {
  struct tm tmv = {};
  tmv.tm_year = dt->year - 1900;
  tmv.tm_mon = dt->month - 1;
  tmv.tm_mday = dt->day;
  tmv.tm_hour = dt->hour;
  tmv.tm_min = dt->min;
  tmv.tm_sec = dt->sec;
  time_t target = timegm(&tmv);
  g_offset_seconds = (long)(target - time(nullptr));
  return true;
}

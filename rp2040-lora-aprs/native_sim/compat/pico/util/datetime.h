#pragma once

#include <cstdint>

struct datetime_t {
  int16_t year;
  int8_t month;
  int8_t day;
  int8_t dotw;
  int8_t hour;
  int8_t min;
  int8_t sec;
};

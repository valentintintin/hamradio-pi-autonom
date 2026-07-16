#include "GpioHal.h"
#include <Arduino.h>

bool GpioHal::pulse(bool level, uint16_t ms) {
  bool rest_level;
  if (!read(rest_level)) {
    return false;
  }
  if (!set(level)) {
    return false;
  }
  delay(ms);
  return set(rest_level);
}

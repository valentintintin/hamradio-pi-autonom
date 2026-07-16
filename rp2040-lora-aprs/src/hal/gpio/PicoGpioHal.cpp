#include "PicoGpioHal.h"
#include <Arduino.h>

bool PicoGpioHal::begin() {
  pinMode(_pin, _mode);
  return true;
}

bool PicoGpioHal::set(bool level) {
  digitalWrite(_pin, level ? HIGH : LOW);
  return true;
}

bool PicoGpioHal::read(bool& level) {
  level = digitalRead(_pin) == HIGH;
  return true;
}

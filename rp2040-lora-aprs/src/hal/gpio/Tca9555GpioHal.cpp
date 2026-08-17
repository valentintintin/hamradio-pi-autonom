#include "Tca9555GpioHal.h"

bool Tca9555GpioHal::begin() {
  return _expander->setPinMode(_pin, _asOutput);
}

bool Tca9555GpioHal::set(bool level) {
  return _expander->writePin(_pin, level);
}

bool Tca9555GpioHal::read(bool& level) {
  return _expander->readPin(_pin, level);
}

bool Tca9555GpioHal::pulse(bool level, uint16_t ms) {
  // Redéfinie car le verrou I2C doit couvrir toute la séquence : sinon une
  // transaction concurrente sur le même port pourrait raccourcir l'impulsion.
  return _expander->pulsePin(_pin, level, ms);
}

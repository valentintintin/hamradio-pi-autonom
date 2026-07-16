#include "Tca9555Hal.h"
#include <Arduino.h>

bool Tca9555Hal::writeRegister(uint8_t reg, uint8_t value) {
  if (!_bus->lock()) {
    return false;
  }
  bool ok = writeRegisterNoLock(reg, value);
  _bus->unlock();
  return ok;
}

bool Tca9555Hal::writeRegisterNoLock(uint8_t reg, uint8_t value) {
  _bus->wire().beginTransmission(_addr);
  _bus->wire().write(reg);
  _bus->wire().write(value);
  return _bus->wire().endTransmission() == 0;
}

bool Tca9555Hal::readRegister(uint8_t reg, uint8_t& value) {
  if (!_bus->lock()) {
    return false;
  }
  bool ok = readRegisterNoLock(reg, value);
  _bus->unlock();
  return ok;
}

bool Tca9555Hal::readRegisterNoLock(uint8_t reg, uint8_t& value) {
  _bus->wire().beginTransmission(_addr);
  _bus->wire().write(reg);
  if (_bus->wire().endTransmission(false) != 0) {
    return false;
  }
  if (_bus->wire().requestFrom(_addr, (uint8_t)1) != 1) {
    return false;
  }
  value = _bus->wire().read();
  return true;
}

bool Tca9555Hal::setPinMode(uint8_t pin, bool asOutput) {
  uint8_t reg = TCA9555_REG_CONFIG_P0 + (pin / 8);
  uint8_t bit_mask = (uint8_t)(1 << (pin % 8));

  if (!_bus->lock()) {
    return false;
  }
  uint8_t value;
  bool ok = readRegisterNoLock(reg, value);
  if (ok) {
    // Registre config TCA9555 : 0 = sortie, 1 = entrée.
    value = asOutput ? (value & ~bit_mask) : (value | bit_mask);
    ok = writeRegisterNoLock(reg, value);
  }
  _bus->unlock();
  return ok;
}

bool Tca9555Hal::writePin(uint8_t pin, bool level) {
  uint8_t reg = TCA9555_REG_OUTPUT_P0 + (pin / 8);
  uint8_t bit_mask = (uint8_t)(1 << (pin % 8));

  if (!_bus->lock()) {
    return false;
  }
  uint8_t value;
  bool ok = readRegisterNoLock(reg, value);
  if (ok) {
    value = level ? (value | bit_mask) : (value & ~bit_mask);
    ok = writeRegisterNoLock(reg, value);
  }
  _bus->unlock();
  return ok;
}

bool Tca9555Hal::readPin(uint8_t pin, bool& level) {
  uint8_t value;
  if (!readRegister(TCA9555_REG_INPUT_P0 + (pin / 8), value)) {
    return false;
  }
  level = (value & (1 << (pin % 8))) != 0;
  return true;
}

bool Tca9555Hal::pulsePin(uint8_t pin, bool level, uint16_t ms) {
  uint8_t reg = TCA9555_REG_OUTPUT_P0 + (pin / 8);
  uint8_t bit_mask = (uint8_t)(1 << (pin % 8));

  if (!_bus->lock()) {
    return false;
  }

  uint8_t rest_value;
  bool ok = readRegisterNoLock(reg, rest_value);
  if (ok) {
    uint8_t pulsed_value = level ? (rest_value | bit_mask) : (rest_value & ~bit_mask);
    ok = writeRegisterNoLock(reg, pulsed_value);
  }
  if (ok) {
    delay(ms);
    ok = writeRegisterNoLock(reg, rest_value);
  }

  _bus->unlock();
  return ok;
}

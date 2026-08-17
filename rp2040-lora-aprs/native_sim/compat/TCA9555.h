#pragma once

#include <Arduino.h>
#include "SimWorld.h"

enum TCA9555_ERROR_t { TCA9555_OK = 0, TCA9555_PIN_ERROR = -1, TCA9555_I2C_ERROR = -2 };

class TCA9555 {
public:
  TCA9555(uint8_t addr, TwoWire* /*wire*/) : _addr(addr) {}

  bool isConnected() { return SimWorld::instance().tca9555_present; }

  bool write8(uint8_t port, uint8_t value) {
    auto& w = SimWorld::instance();
    std::lock_guard<std::mutex> lock(w.mutex);
    uint16_t mask = (uint16_t)0xFF << (port * 8);
    w.tca9555_output = (uint16_t)((w.tca9555_output & ~mask) | ((uint16_t)value << (port * 8)));
    _lastError = TCA9555_OK;
    return true;
  }

  bool pinMode8(uint8_t port, uint8_t configMask) {
    auto& w = SimWorld::instance();
    std::lock_guard<std::mutex> lock(w.mutex);
    uint16_t mask = (uint16_t)0xFF << (port * 8);
    w.tca9555_config = (uint16_t)((w.tca9555_config & ~mask) | ((uint16_t)configMask << (port * 8)));
    _lastError = TCA9555_OK;
    return true;
  }

  bool pinMode1(uint8_t pin, uint8_t mode) {
    auto& w = SimWorld::instance();
    std::lock_guard<std::mutex> lock(w.mutex);
    uint16_t bit = (uint16_t)1 << pin;
    if (mode == OUTPUT) {
      w.tca9555_config &= ~bit;
    } else {
      w.tca9555_config |= bit;
    }
    _lastError = TCA9555_OK;
    return true;
  }

  bool write1(uint8_t pin, uint8_t level) {
    auto& w = SimWorld::instance();
    std::lock_guard<std::mutex> lock(w.mutex);
    uint16_t bit = (uint16_t)1 << pin;
    if (level == HIGH) {
      w.tca9555_output |= bit;
    } else {
      w.tca9555_output &= ~bit;
    }
    _lastError = TCA9555_OK;
    return true;
  }

  // Reflète l'état électrique réel de la broche même en sortie, comme le vrai chip.
  uint8_t read1(uint8_t pin) {
    auto& w = SimWorld::instance();
    std::lock_guard<std::mutex> lock(w.mutex);
    _lastError = TCA9555_OK;
    return (w.tca9555_output & ((uint16_t)1 << pin)) ? HIGH : LOW;
  }

  int lastError() { return _lastError; }

private:
  uint8_t _addr;
  int _lastError = TCA9555_OK;
};

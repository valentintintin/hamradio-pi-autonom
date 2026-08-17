#include "hal/gpio/Tca9555Hal.h"
#include <Arduino.h>

bool Tca9555Hal::isConnected() {
  if (!_bus->lock()) {
    return false;
  }
  bool ok = _tca.isConnected();
  _bus->unlock();
  return ok;
}

bool Tca9555Hal::writeOutputPort(uint8_t port, uint8_t value) {
  if (!_bus->lock()) {
    return false;
  }
  bool ok = _tca.write8(port, value);
  _bus->unlock();
  return ok;
}

bool Tca9555Hal::writeConfigPort(uint8_t port, uint8_t configMask) {
  if (!_bus->lock()) {
    return false;
  }
  bool ok = _tca.pinMode8(port, configMask);
  _bus->unlock();
  return ok;
}

bool Tca9555Hal::setPinMode(uint8_t pin, bool asOutput) {
  if (!_bus->lock()) {
    return false;
  }
  bool ok = _tca.pinMode1(pin, asOutput ? OUTPUT : INPUT);
  _bus->unlock();
  return ok;
}

bool Tca9555Hal::writePin(uint8_t pin, bool level) {
  if (!_bus->lock()) {
    return false;
  }
  bool ok = _tca.write1(pin, level ? HIGH : LOW);
  _bus->unlock();
  return ok;
}

bool Tca9555Hal::readPin(uint8_t pin, bool& level) {
  if (!_bus->lock()) {
    return false;
  }
  uint8_t value = _tca.read1(pin);
  bool ok = (_tca.lastError() == TCA9555_OK);
  _bus->unlock();
  if (ok) {
    level = (value == HIGH);
  }
  return ok;
}

bool Tca9555Hal::pulsePin(uint8_t pin, bool level, uint16_t ms) {
  if (!_bus->lock()) {
    return false;
  }

  // read1() lit le registre d'entrée, qui reflète l'état électrique réel
  // même en sortie — donne l'état "avant impulsion" à restaurer.
  bool rest_level = (_tca.read1(pin) == HIGH);
  bool ok = (_tca.lastError() == TCA9555_OK);
  if (ok) {
    ok = _tca.write1(pin, level ? HIGH : LOW);
  }
  if (ok) {
    delay(ms);
    ok = _tca.write1(pin, rest_level ? HIGH : LOW);
  }

  _bus->unlock();
  return ok;
}

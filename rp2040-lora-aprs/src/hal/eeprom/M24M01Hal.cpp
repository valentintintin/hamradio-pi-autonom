#include "M24M01Hal.h"
#include <Arduino.h>
#include <M24M01.h>

// Variante native (pas de bus I2C réel) : native_sim/sim/M24M01HalSim.cpp,
// exclu/inclus par platformio.ini selon l'env — pas de #ifdef ici.

bool M24M01Hal::begin() {
  if (!_bus->lock()) {
    return false;
  }
  if (!_dev) {
    _dev = new M24M01(_bus->wire(), _addr);
  }
  _initialized = _dev->begin();
  _bus->unlock();
  return _initialized;
}

bool M24M01Hal::read(uint32_t address, uint8_t* data, size_t len) {
  if (!_initialized) {
    return false;
  }
  if (!_bus->lock()) {
    return false;
  }
  bool ok = _dev->read(address, data, len);
  _bus->unlock();
  return ok;
}

bool M24M01Hal::write(uint32_t address, const uint8_t* data, size_t len) {
  if (!_initialized) {
    return false;
  }
  if (!_bus->lock()) {
    return false;
  }
  bool ok = _dev->write(address, data, len);
  _bus->unlock();
  return ok;
}

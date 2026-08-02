#include "M24M01Hal.h"
#include <Arduino.h>
#include <M24M01.h>

// ============================================================================
// M24M01Hal — implémentation matérielle réelle : délègue à la lib externe
// M24M01 (bus I2C via Wire), en ajoutant le verrouillage du bus (I2CBus,
// mutex partagé avec les autres périphériques I2C) autour de chaque appel.
// La variante native (env PlatformIO `native`, pas de bus I2C réel —
// persistée dans de vrais fichiers JSON sous SIM_DATA_DIR/eeprom/) vit dans
// native_sim/sim/M24M01HalSim.cpp ; platformio.ini exclut ce fichier-ci pour
// cet env et compile l'autre à la place (même principe que variant/ vs
// variant_native/ pour les radios) — aucun #ifdef NATIVE_BUILD nécessaire
// ici.
// ============================================================================

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

#pragma once

// ============================================================================
// Adafruit_INA3221.h — faux vendor header natif (cf. Adafruit_BME280.h) —
// hal/sensors/Ina3221Hal.h reste inchangé. Mapping des 3 canaux identique à
// Ina3221Hal::query() : 0=batterie, 1=board (telemetry.solar_ina), 2=solaire
// (telemetry.board_5v) — cf. commentaires de ce fichier, reproduits tels
// quels ici plutôt que "corrigés" : le simulateur doit refléter le
// comportement réel du code, pas une intention supposée.
// ============================================================================

#include <Arduino.h>
#include "SimWorld.h"

class Adafruit_INA3221 {
public:
  bool begin(uint8_t /*addr*/, TwoWire* /*wire*/) { return true; }

  float getBusVoltage(int channel) {
    auto& w = SimWorld::instance();
    std::lock_guard<std::mutex> lock(w.mutex);
    switch (channel) {
      case 0: return w.battery_mv / 1000.0f;
      case 1: return w.board5v_mv / 1000.0f;
      case 2: return w.solar_ina_mv / 1000.0f;
      default: return 0.0f;
    }
  }

  float getCurrentAmps(int channel) {
    auto& w = SimWorld::instance();
    std::lock_guard<std::mutex> lock(w.mutex);
    switch (channel) {
      case 0: return w.battery_ma / 1000.0f;
      case 1: return w.board5v_ma / 1000.0f;
      case 2: return w.solar_ina_ma / 1000.0f;
      default: return 0.0f;
    }
  }
};

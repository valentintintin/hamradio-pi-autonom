#pragma once

// Ina3221Hal::query() stocke le canal 1 ("board") dans telemetry.solar_ina et
// le canal 2 ("solar") dans telemetry.board_5v (bug d'origine) — reproduit
// tel quel ici, le simulateur doit refléter le comportement réel, pas l'intention.
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

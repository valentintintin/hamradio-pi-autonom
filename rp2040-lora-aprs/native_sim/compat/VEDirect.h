#pragma once

#include <Arduino.h>
#include "SimWorld.h"
#include <cstdint>

enum VeDirectField {
  VE_VOLTAGE,
  VE_CURRENT,
  VE_PANEL_VOLTAGE,
  VE_PANEL_POWER,
  VE_STATE_OF_OPERATION,
  VE_SOC
};

class VEDirect {
public:
  explicit VEDirect(HardwareSerial& /*serial*/) {}

  bool begin() { return SimWorld::instance().victron_present; }
  void update() {}
  bool available() { return SimWorld::instance().victron_present; }

  int32_t read(VeDirectField field) {
    auto& w = SimWorld::instance();
    std::lock_guard<std::mutex> lock(w.mutex);
    switch (field) {
      case VE_VOLTAGE:            return (int32_t)w.mppt_battery_mv;
      case VE_CURRENT:            return (int32_t)w.mppt_battery_ma;
      case VE_PANEL_VOLTAGE:      return (int32_t)w.mppt_solar_mv;
      case VE_PANEL_POWER:        return (int32_t)(w.mppt_solar_mv * w.mppt_solar_ma / 1000.0f);
      case VE_STATE_OF_OPERATION: return w.victron_state;
      case VE_SOC:                return (int32_t)(w.victron_soc_pct * 10.0f);  // 0.1%
    }
    return 0;
  }
};

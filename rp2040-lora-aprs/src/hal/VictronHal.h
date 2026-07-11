#pragma once

#include "Telemetry.h"
#include <VEDirect.h>

// ============================================================================
// Victron VE.Direct HAL — lecture batterie via protocole texte VE.Direct
// Connecté sur un UART (Serial1/Serial2) à 19200 baud
// ============================================================================

class VictronHal {
public:
  VictronHal(HardwareSerial& serial) : _ved(serial), _initialized(false) {}

  bool begin() {
    _initialized = _ved.begin();
    return _initialized;
  }

  bool query(TelemetryData& telemetry) {
    if (!_initialized) return false;

    // Tension batterie (mV → mV, la lib retourne directement en mV)
    int32_t v = _ved.read(VE_VOLTAGE);
    if (v > 0) telemetry.victron_voltage_mv = (float)v;

    // Courant (mA)
    int32_t i = _ved.read(VE_CURRENT);
    telemetry.victron_current_ma = (float)i;

    // Puissance (W)
    int32_t p = _ved.read(VE_POWER);
    telemetry.victron_power_w = (float)p;

    // State of charge (0.1% → %)
    int32_t soc = _ved.read(VE_SOC);
    if (soc >= 0) telemetry.victron_soc = soc / 10.0f;

    return true;
  }

  bool isInitialized() const { return _initialized; }

private:
  VEDirect _ved;
  bool _initialized;
};

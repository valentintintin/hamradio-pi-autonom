#pragma once

#include "Telemetry.h"
#include "ChargeControllerHal.h"
#include <VEDirect.h>

// ============================================================================
// Victron VE.Direct HAL — lecture batterie via protocole texte VE.Direct
// Connecté sur un UART (Serial1/Serial2) à 19200 baud
// https://www.victronenergy.com/upload/documents/VE.Direct-Protocol-3.34.pdf
// ============================================================================

class VictronHal : public ChargeControllerHal {
public:
  VictronHal(HardwareSerial& serial) : _ved(serial), _initialized(false) {}

  bool begin() override {
    _initialized = _ved.begin();
    return _initialized;
  }

  bool query(TelemetryData& telemetry) override {
    if (!_initialized) {
      return false;
    }

    _ved.update();

    if (!_ved.available()) {
      return false;
    }

    // Tension batterie (mV → mV, la lib retourne directement en mV)
    int32_t v = _ved.read(VE_VOLTAGE);
    if (v > 0) {
      telemetry.battery_mppt.voltage_mv = (float)v;
    }

    int32_t i = _ved.read(VE_CURRENT);
    telemetry.battery_mppt.current_ma = (float)i;

    i = _ved.read(VE_PANEL_VOLTAGE);
    telemetry.solar_mppt.voltage_mv = (float)i;

    i = _ved.read(VE_PANEL_POWER) / i;
    telemetry.solar_mppt.current_ma = (float)i;

    i = _ved.read(VE_STATE_OF_OPERATION);
    telemetry.mppt_status = i;

    // State of charge (0.1% → %)
    int32_t soc = _ved.read(VE_SOC);
    if (soc >= 0) {
      telemetry.victron_soc = soc / 10.0f;
    }

    return true;
  }

  bool isInitialized() const override { return _initialized; }

private:
  VEDirect _ved;
  bool _initialized;
};

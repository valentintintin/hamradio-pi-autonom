#pragma once

#include "I2CBus.h"
#include "Telemetry.h"
#include <mpptChg.h>

// ============================================================================
// MakePower MPPT Charger HAL — I2C addr 0x12
// Lecture batterie, solaire, température, watchdog
// ============================================================================

class MpptChargerHal {
public:
  MpptChargerHal(I2CBus& bus) : _bus(&bus), _initialized(false) {}

  bool begin() {
    if (!_bus->lock()) {
      return false;
    }
    _initialized = _mppt.begin(_bus->wire());
    _bus->unlock();
    return _initialized;
  }

  bool query(TelemetryData& telemetry) {
    if (!_initialized) {
      return false;
    }
    if (!_bus->lock()) {
      return false;
    }

    int16_t val;
    uint16_t uval;

    if (_mppt.getIndexedValue(VAL_VB, &val)) {
      telemetry.battery.voltage_mv = val;
    }
    if (_mppt.getIndexedValue(VAL_IB, &val)) {
      telemetry.battery.current_ma = val;
    }
    if (_mppt.getIndexedValue(VAL_VS, &val)) {
      telemetry.solar.voltage_mv = val;
    }
    if (_mppt.getIndexedValue(VAL_IS, &val)) {
      telemetry.solar.current_ma = val;
    }
    if (_mppt.getIndexedValue(VAL_IC, &val)) {
      telemetry.mppt_charge_ma = val;
    }
    if (_mppt.getIndexedValue(VAL_INT_TEMP, &val)) {
      telemetry.mppt_int_temp_c = val / 10.0f;
    }

    if (_mppt.getStatusValue(SYS_STATUS, &uval)) {
      telemetry.mppt_status = uval;
    }

    bool alert = false;
    bool night = false;
    _mppt.isAlert(&alert);
    _mppt.isNight(&night);
    telemetry.mppt_alert = alert;
    telemetry.mppt_night = night;

    _bus->unlock();
    return true;
  }

  // Nourrir le watchdog (appeler périodiquement)
  bool feedWatchdog(uint8_t timeout_s = 120) {
    if (!_initialized) {
      return false;
    }
    if (!_bus->lock()) {
      return false;
    }
    bool ok = _mppt.setWatchdogTimeout(timeout_s);
    _bus->unlock();
    return ok;
  }

  bool enableWatchdog(bool enable) {
    if (!_initialized) {
      return false;
    }
    if (!_bus->lock()) {
      return false;
    }
    bool ok = _mppt.setWatchdogEnable(enable);
    _bus->unlock();
    return ok;
  }

  bool isInitialized() const { return _initialized; }

private:
  I2CBus* _bus;
  bool _initialized;
  mpptChg _mppt;
};

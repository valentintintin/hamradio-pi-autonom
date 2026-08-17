#pragma once

#include "hal/i2c/I2CBus.h"
#include "hal/Telemetry.h"
#include "ChargeControllerHal.h"
#include <mpptChg.h>

// MakePower MPPT Charger, I2C addr 0x12.

// Pin d'interruption "extinction imminente" de la carte MPPT (actif haut).
// TODO: ajuster selon le câblage réel de la carte.
#define MPPT_ALERT_PIN 26

class MpptChargerHal : public ChargeControllerHal {
public:
  MpptChargerHal(I2CBus& bus) : _bus(&bus), _initialized(false) {}

  bool begin() override {
    if (!_bus->lock()) {
      return false;
    }
    _initialized = _mppt.begin(_bus->wire());
    _bus->unlock();
    return _initialized;
  }

  bool query(TelemetryData& telemetry) override {
    if (!_initialized) {
      return false;
    }
    if (!_bus->lock()) {
      return false;
    }

    int16_t val;
    uint16_t uval;

    if (_mppt.getIndexedValue(VAL_VB, &val)) {
      telemetry.battery_mppt.voltage_mv = val;
    }
    if (_mppt.getIndexedValue(VAL_IB, &val)) {
      telemetry.battery_mppt.current_ma = val;
    }
    if (_mppt.getIndexedValue(VAL_VS, &val)) {
      telemetry.solar_mppt.voltage_mv = val;
    }
    if (_mppt.getIndexedValue(VAL_IS, &val)) {
      telemetry.solar_mppt.current_ma = val;
    }

    if (_mppt.getStatusValue(SYS_STATUS, &uval)) {
      telemetry.mppt_status = uval;
    }

    _bus->unlock();
    return true;
  }

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

  // Lecture I2C de l'ALERT, redondante avec le GPIO MPPT_ALERT_PIN — utile
  // si l'interruption matérielle est manquée ou non câblée.
  bool isAlertEnabled(bool& alert) {
    if (!_initialized) {
      return false;
    }
    if (!_bus->lock()) {
      return false;
    }
    bool ok = _mppt.isAlert(&alert);
    _bus->unlock();
    return ok;
  }

  bool setPowerOffThreshold(uint16_t mv) {
    if (!_initialized) {
      return false;
    }
    if (!_bus->lock()) {
      return false;
    }
    bool ok = _mppt.setConfigurationValue(CFG_PWR_OFF_TH, mv);
    _bus->unlock();
    return ok;
  }

  bool getPowerOffThreshold(uint16_t& mv) {
    if (!_initialized) {
      return false;
    }
    if (!_bus->lock()) {
      return false;
    }
    bool ok = _mppt.getConfigurationValue(CFG_PWR_OFF_TH, &mv);
    _bus->unlock();
    return ok;
  }

  bool setPowerOnThreshold(uint16_t mv) {
    if (!_initialized) {
      return false;
    }
    if (!_bus->lock()) {
      return false;
    }
    bool ok = _mppt.setConfigurationValue(CFG_PWR_ON_TH, mv);
    _bus->unlock();
    return ok;
  }

  bool getPowerOnThreshold(uint16_t& mv) {
    if (!_initialized) {
      return false;
    }
    if (!_bus->lock()) {
      return false;
    }
    bool ok = _mppt.getConfigurationValue(CFG_PWR_ON_TH, &mv);
    _bus->unlock();
    return ok;
  }

  bool isInitialized() const override { return _initialized; }

private:
  I2CBus* _bus;
  bool _initialized;
  mpptChg _mppt;
};

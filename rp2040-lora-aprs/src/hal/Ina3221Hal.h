#pragma once

#include "I2CBus.h"
#include "Telemetry.h"
#include <Adafruit_INA3221.h>

// ============================================================================
// INA3221 HAL — triple monitoring courant/tension
// Canaux : 0=battery, 1=board, 2=solar (shunts 33mΩ)
// ============================================================================

#define INA3221_SHUNT_MOHM  33

class Ina3221Hal {
public:
  Ina3221Hal(I2CBus& bus, uint8_t addr = 0x40)
    : _bus(&bus), _addr(addr), _initialized(false) {}

  bool begin() {
    if (!_bus->lock()) return false;
    _initialized = _ina.begin(_addr, &_bus->wire());
    _bus->unlock();
    return _initialized;
  }

  bool query(TelemetryData& telemetry) {
    if (!_initialized) return false;
    if (!_bus->lock()) return false;

    // Canal 0 : batterie
    telemetry.battery.voltage_mv = _ina.getBusVoltage(0) * 1000.0f;
    telemetry.battery.current_ma = _ina.getCurrentAmps(0) * 1000.0f;

    // Canal 1 : board
    telemetry.board.voltage_mv = _ina.getBusVoltage(1) * 1000.0f;
    telemetry.board.current_ma = _ina.getCurrentAmps(1) * 1000.0f;

    // Canal 2 : solaire
    telemetry.solar.voltage_mv = _ina.getBusVoltage(2) * 1000.0f;
    telemetry.solar.current_ma = _ina.getCurrentAmps(2) * 1000.0f;

    _bus->unlock();
    return true;
  }

  bool isInitialized() const { return _initialized; }

private:
  I2CBus* _bus;
  uint8_t _addr;
  bool _initialized;
  Adafruit_INA3221 _ina;
};

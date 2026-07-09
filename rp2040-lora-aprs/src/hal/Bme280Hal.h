#pragma once

#include "I2CBus.h"
#include "Telemetry.h"
#include <Adafruit_BME280.h>

// ============================================================================
// BME280 HAL — température, humidité, pression
// Forced mode, low power
// ============================================================================

class Bme280Hal {
public:
  Bme280Hal(I2CBus& bus, uint8_t addr = 0x76)
    : _bus(&bus), _addr(addr), _initialized(false) {}

  bool begin() {
    if (!_bus->lock()) return false;
    _initialized = _bme.begin(_addr, &_bus->wire());
    if (_initialized) {
      _bme.setSampling(
        Adafruit_BME280::MODE_FORCED,
        Adafruit_BME280::SAMPLING_X1,  // temp
        Adafruit_BME280::SAMPLING_X1,  // pressure
        Adafruit_BME280::SAMPLING_X1,  // humidity
        Adafruit_BME280::FILTER_OFF,
        Adafruit_BME280::STANDBY_MS_1000
      );
    }
    _bus->unlock();
    return _initialized;
  }

  bool query(WeatherData& data) {
    if (!_initialized) return false;
    if (!_bus->lock()) return false;

    _bme.takeForcedMeasurement();
    data.temperature_c = _bme.readTemperature();
    data.humidity = _bme.readHumidity();
    data.pressure_hpa = _bme.readPressure() / 100.0f;

    _bus->unlock();
    return true;
  }

  bool isInitialized() const { return _initialized; }

private:
  I2CBus* _bus;
  uint8_t _addr;
  bool _initialized;
  Adafruit_BME280 _bme;
};

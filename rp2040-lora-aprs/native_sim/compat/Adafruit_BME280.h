#pragma once

#include <Arduino.h>
#include "SimWorld.h"

class Adafruit_BME280 {
public:
  enum sensor_mode { MODE_SLEEP = 0, MODE_FORCED = 1, MODE_NORMAL = 3 };
  enum sensor_sampling { SAMPLING_NONE = 0, SAMPLING_X1 = 1, SAMPLING_X2, SAMPLING_X4, SAMPLING_X8, SAMPLING_X16 };
  enum sensor_filter { FILTER_OFF = 0, FILTER_X2, FILTER_X4, FILTER_X8, FILTER_X16 };
  enum standby_duration { STANDBY_MS_0_5 = 0, STANDBY_MS_1000 = 5 };

  bool begin(uint8_t /*addr*/, TwoWire* /*wire*/) {
    return SimWorld::instance().temperature_c > -273.0f;
  }

  void setSampling(sensor_mode, sensor_sampling, sensor_sampling, sensor_sampling,
                    sensor_filter, standby_duration) {}

  void takeForcedMeasurement() {}

  float readTemperature() {
    auto& w = SimWorld::instance();
    std::lock_guard<std::mutex> lock(w.mutex);
    return w.temperature_c;
  }

  float readHumidity() {
    auto& w = SimWorld::instance();
    std::lock_guard<std::mutex> lock(w.mutex);
    return w.humidity_pct;
  }

  // Retourne des Pa: Bme280Hal divise par 100 pour obtenir des hPa, comme la vraie lib.
  float readPressure() {
    auto& w = SimWorld::instance();
    std::lock_guard<std::mutex> lock(w.mutex);
    return w.pressure_hpa * 100.0f;
  }
};

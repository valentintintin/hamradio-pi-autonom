#pragma once

#include "hal/Telemetry.h"

class ChargeControllerHal {
public:
  virtual ~ChargeControllerHal() = default;

  virtual bool begin() = 0;
  virtual bool query(TelemetryData& telemetry) = 0;
  virtual bool isInitialized() const = 0;
};

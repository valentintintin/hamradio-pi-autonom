#pragma once

#include <helpers/sensors/EnvironmentSensorManager.h>

class SensorController : EnvironmentSensorManager {
protected:
    bool mpptChg_initialized = false;
public:
    bool begin() override;

    bool querySensors(uint8_t requester_permissions, CayenneLPP &telemetry) override;
};

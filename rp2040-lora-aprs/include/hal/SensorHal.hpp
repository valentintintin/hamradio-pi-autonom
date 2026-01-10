#pragma once

#include "telemetry.h"

#include <ArduinoLog.h>
#include "controllers/LedController.hpp"

class SensorHal
{
public:
    virtual bool begin() = 0;
    virtual bool query(Telemetry &telemetry) = 0;

    bool isInitialized() const
    {
        return initialized;
    }

    virtual ~SensorHal() = default;
protected:
    bool initialized = false;
    SensorHal();
};

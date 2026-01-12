#pragma once

#include "telemetry.h"
#include "hal/Hal.hpp"

class SensorHal : public Hal
{
public:
    virtual bool query(Telemetry &telemetry) = 0;
protected:
    SensorHal() = default;
};

#pragma once

#include "hal/SensorHal.hpp"

#include "Adafruit_INA3221.h"

class Ina3221Hal : public SensorHal
{
public:
    static Ina3221Hal& getInstance()
    {
        static Ina3221Hal instance;
        return instance;
    }

    bool begin() override;
    bool query(Telemetry &telemetry) override;
private:
    Adafruit_INA3221 ina3221 = Adafruit_INA3221();
};

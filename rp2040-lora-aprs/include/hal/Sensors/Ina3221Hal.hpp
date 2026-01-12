#pragma once

#include "SensorHal.hpp"

#include "Adafruit_INA3221.h"

class Ina3221Hal : public SensorHal
{
public:
    static Ina3221Hal& getInstance()
    {
        static Ina3221Hal instance;
        return instance;
    }

    bool query(Telemetry &telemetry) override;
protected:
    bool doBegin() override;
private:
    Adafruit_INA3221 ina3221 = Adafruit_INA3221();
};

#pragma once
#include "telemetry.h"
#include "controllers/I2CSlaveController.hpp"

class I2CSlaveHal
{
public:
    bool begin(uint8_t address);
    bool queryTelemetries(Telemetry& telemetry);
    bool queryClock(uint32_t& now);
private:
    bool initialized = false;
    uint8_t slaveAddress = 0;

    void setRegisterToRead(I2CSlaveRegisterValue reg) const;
    bool readRegister(I2CSlaveRegisterValue reg, void* dest, size_t size);
};

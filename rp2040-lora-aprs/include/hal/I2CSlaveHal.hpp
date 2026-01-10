#pragma once
#include "SensorHal.hpp"
#include "telemetry.h"
#include "i2c_slave.h"

class I2CSlaveHal : public SensorHal
{
public:
    static I2CSlaveHal& getInstance()
    {
        static I2CSlaveHal instance;
        return instance;
    }

    bool begin() override;
    bool query(Telemetry& telemetry) override;

    bool queryClock(uint32_t& now) const;
private:
    void setRegisterToRead(I2CSlaveRegisterValue reg) const;
    bool readRegister(I2CSlaveRegisterValue reg, void* dest, size_t size) const;
};

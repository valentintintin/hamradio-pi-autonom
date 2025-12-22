#pragma once

#include "BaseController.hpp"
#include "telemetry.h"

#define I2C_ADDR 0x10

class I2CSlaveController : BaseController
{
public:
    static I2CSlaveController& getInstance()
    {
        static I2CSlaveController instance;
        return instance;
    }

    bool begin() override;
private:
    static volatile uint16_t regIndex;

    static void onReceive(int regIndexWanted);
    static void onRequest();
};

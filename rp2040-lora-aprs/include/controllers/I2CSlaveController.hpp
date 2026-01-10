#pragma once

#include "BaseController.hpp"
#include "telemetry.h"
#include "i2c_slave.h"

class I2CSlaveController : public BaseController
{
public:
    static I2CSlaveController& getInstance()
    {
        static I2CSlaveController instance;
        return instance;
    }

    bool begin() override;
private:
    static void onReceive(int howMany);
    static void onRequest();

    static uint8_t txBuffer[I2C_BUFFER_SIZE];
    static size_t txBufferSize;

    static void preparePongBuffer();
    static void prepareClockBuffer();
    static void prepareTelemetryBuffer(I2CSlaveRegisterValue what);

    static bool fillBuffer(const void *buffer, size_t size);
};

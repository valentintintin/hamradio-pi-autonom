#pragma once

#include "BaseController.hpp"
#include "telemetry.h"

#define I2C_OK 0xAA
#define I2C_KO 0xEE

#define I2C_BUFFER_SIZE 32

enum I2CSlaveRegisterValue
{
    RegisterPing,
    RegisterTelemetryUpdatedAt,
    RegisterTelemetryBattery,
    RegisterTelemetrySolar,
    RegisterTelemetryBox,
    RegisterTelemetryOutdoorBasic,
    RegisterTelemetryOutdoorRain,
    RegisterTelemetryOutdoorWind,
    RegisterTelemetryOutdoorLight,
    RegisterTelemetryTempBatteryTemperature,
    RegisterPosition,
    RegisterClock
};

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

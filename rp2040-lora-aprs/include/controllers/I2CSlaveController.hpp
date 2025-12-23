#pragma once

#include "BaseController.hpp"
#include "telemetry.h"

#define I2C_OK 0xAA
#define I2C_KO 0xEE

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
    static void onReceive(int howMany);
    static void onRequest();

    I2CSlaveRegisterValue regIndex = RegisterPing;

    void sendPong() const;
    void sendClock() const;
    void sendTelemetry(I2CSlaveRegisterValue what) const;
};

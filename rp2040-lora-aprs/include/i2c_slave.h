#pragma once

#define I2C_BUFFER_SIZE 32

#define I2C_OK 0xAA
#define I2C_KO 0xEE

enum I2CSlaveRegisterValue
{
    RegisterPing,
    RegisterTelemetryUpdatedAt,
    RegisterTelemetryBattery,
    RegisterTelemetrySolar,
    RegisterTelemetryBox,
    RegisterTelemetryOutdoor,
    RegisterTelemetryOutdoorRain,
    RegisterTelemetryOutdoorWind,
    RegisterTelemetryOutdoorLight,
    RegisterTelemetryBatteryTemperature,
    RegisterPosition,
    RegisterClock
};
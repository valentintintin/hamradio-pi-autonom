#include "controllers/I2CSlaveController.hpp"

#include <Wire.h>
#include <ArduinoLog.h>

#include "SettingsManager.hpp"
#include "controllers/SensorController.hpp"
#include "utils/utils.h"

bool I2CSlaveController::begin()
{
    const auto [enabled, address] = SettingsManager::getSettings().i2c;

    if (!enabled)
    {
        Log.noticeln("I2C Slave not enabled");
        return false;
    }

    // if (!Wire1.setSDA(2)) {
    //     Log.errorln("I2C Slave init failed, wrong SDA pin");
    //
    //     return false;
    // }
    //
    // if (!Wire1.setSCL(3)) {
    //     Log.errorln("I2C Slave init failed, wrong SCL pin");
    //
    //     return false;
    // }

    Wire1.begin(address);
    Wire1.onReceive(onReceive);
    Wire1.onRequest(onRequest);

    Log.infoln("I2C Slave init on address %X OK", address);

    return true;
}

void I2CSlaveController::onReceive(const int howMany)
{
    Log.infoln("I2C Slave, index wants %d", howMany);

    getInstance().regIndex = static_cast<I2CSlaveRegisterValue>(howMany);

    // Flush
    while (Wire1.available())
    {
        Wire1.read();
    }
}

void I2CSlaveController::onRequest()
{
    const auto regIndex = getInstance().regIndex;

    switch (regIndex)
    {
    case RegisterPing:
        getInstance().sendPong();
        break;
    case RegisterClock:
        getInstance().sendClock();
        break;
    default:
        getInstance().sendTelemetry(regIndex);
        break;
    }
}

void I2CSlaveController::sendPong() const
{
    Wire1.write(I2C_OK);
}

void I2CSlaveController::sendClock() const
{
    auto now = getDateTime().unixtime();
    Wire1.write(reinterpret_cast<uint8_t*>(&now), sizeof(now));
}

void I2CSlaveController::sendTelemetry(I2CSlaveRegisterValue what) const
{
    auto pointer = &SensorController::getTelemetry();
    size_t size = 1;

    switch (regIndex)
    {
    case RegisterTelemetryUpdatedAt:
        pointer += offsetof(Telemetry, updatedAt);
        size = sizeof(decltype(Telemetry::updatedAt));
        break;
    case RegisterTelemetryBattery:
        pointer += offsetof(Telemetry, battery);
        size = sizeof(TelemetryPower);
        break;
    case RegisterTelemetrySolar:
        pointer += offsetof(Telemetry, solar);
        size = sizeof(TelemetryPower);
        break;
    case RegisterTelemetryBox:
        pointer += offsetof(Telemetry, box);
        size = sizeof(TelemetryBasic);
        break;
    case RegisterTelemetryOutdoorBasic:
        pointer += offsetof(Telemetry, outdoor) + offsetof(TelemetryOutdoor, basic);
        size = sizeof(TelemetryBasic);
        break;
    case RegisterTelemetryOutdoorRain:
        pointer += offsetof(Telemetry, outdoor) + offsetof(TelemetryOutdoor, rain);
        size = sizeof(decltype(TelemetryOutdoor::rain));
        break;
    case RegisterTelemetryOutdoorWind:
        pointer += offsetof(Telemetry, outdoor) + offsetof(TelemetryOutdoor, wind);
        size = sizeof(TelemetryWind);
        break;
    case RegisterTelemetryOutdoorLight:
        pointer += offsetof(Telemetry, outdoor) + offsetof(TelemetryOutdoor, light);
        size = sizeof(TelemetryLight);
        break;
    case RegisterTelemetryTempBatteryTemperature:
        pointer += offsetof(Telemetry, batteryTemperature);
        size = sizeof(decltype(Telemetry::batteryTemperature));
        break;
    case RegisterPosition:
        pointer += offsetof(Telemetry, position);
        size = sizeof(Position);
        break;
    default:
        break;
    }

    Wire.write(reinterpret_cast<const uint8_t*>(pointer), size);
}

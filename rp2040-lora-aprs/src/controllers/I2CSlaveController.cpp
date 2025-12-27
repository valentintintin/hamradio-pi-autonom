#include "controllers/I2CSlaveController.hpp"

#include <Wire.h>
#include <ArduinoLog.h>

#include "SettingsManager.hpp"
#include "controllers/SensorController.hpp"
#include "utils/utils.h"

uint8_t I2CSlaveController::txBuffer[I2C_BUFFER_SIZE]{};
size_t I2CSlaveController::txBufferSize = 0;

bool I2CSlaveController::begin()
{
    const auto [enabled, address] = SettingsManager::getSettings().i2c;

    if (!enabled)
    {
        Log.noticeln("I2C Slave not enabled");
        return false;
    }

    Wire1.begin(address);
    Wire1.onReceive(onReceive);
    Wire1.onRequest(onRequest);

    Log.infoln("I2C Slave init on address %X OK", address);

    return true;
}

void I2CSlaveController::onReceive(const int howMany)
{
    Log.traceln("I2C Slave, master wants %d bytes", howMany);

    if (howMany == 1)
    {
        const auto regIndex = static_cast<I2CSlaveRegisterValue>(Wire1.read());

        Log.traceln("I2C Slave, received register %d OK", regIndex);

        switch (regIndex)
        {
        case RegisterPing:
            preparePongBuffer();
            break;
        case RegisterClock:
            prepareClockBuffer();
            break;
        default:
            prepareTelemetryBuffer(regIndex);
            break;
        }

        Log.infoln("I2C Slave, register %d OK", regIndex);
    }
}

void I2CSlaveController::onRequest()
{
    if (txBufferSize == 0)
    {
        Log.warningln("I2C Slave, buffer size 0, sent KO");

        return;
    }

    Wire1.write(txBuffer, txBufferSize);

    Log.infoln("I2C Slave, sent %d bytes OK", txBufferSize);
}

void I2CSlaveController::preparePongBuffer()
{
    Log.traceln("Send pong: %X", I2C_OK);
    txBuffer[0] = I2C_OK;
    txBufferSize = 1;
}

void I2CSlaveController::prepareClockBuffer()
{
    const auto now = getDateTime().unixtime();

    Log.traceln("Send clock: %u", now);

    fillBuffer(&now, sizeof(now));
}

void I2CSlaveController::prepareTelemetryBuffer(const I2CSlaveRegisterValue what)
{
    const Telemetry& telemetry = SensorController::getTelemetry();
    auto pointer = reinterpret_cast<const uint8_t*>(&telemetry);
    auto size = 1;

    switch (what)
    {
    case RegisterTelemetryUpdatedAt:
        pointer += offsetof(Telemetry, updatedAt);
        size = sizeof(telemetry.updatedAt);
        break;
    case RegisterTelemetryBattery:
        pointer += offsetof(Telemetry, battery);
        size = sizeof(telemetry.battery);
        break;
    case RegisterTelemetrySolar:
        pointer += offsetof(Telemetry, solar);
        size = sizeof(telemetry.solar);
        break;
    case RegisterTelemetryBox:
        pointer += offsetof(Telemetry, box);
        size = sizeof(telemetry.box);
        break;
    case RegisterTelemetryOutdoorBasic:
        pointer += offsetof(Telemetry, outdoor) + offsetof(TelemetryOutdoor, basic);
        size = sizeof(telemetry.outdoor.basic);
        break;
    case RegisterTelemetryOutdoorRain:
        pointer += offsetof(Telemetry, outdoor) + offsetof(TelemetryOutdoor, rain);
        size = sizeof(telemetry.outdoor.rain);
        break;
    case RegisterTelemetryOutdoorWind:
        pointer += offsetof(Telemetry, outdoor) + offsetof(TelemetryOutdoor, wind);
        size = sizeof(telemetry.outdoor.wind);
        break;
    case RegisterTelemetryOutdoorLight:
        pointer += offsetof(Telemetry, outdoor) + offsetof(TelemetryOutdoor, light);
        size = sizeof(telemetry.outdoor.light);
        break;
    case RegisterTelemetryTempBatteryTemperature:
        pointer += offsetof(Telemetry, batteryTemperature);
        size = sizeof(telemetry.batteryTemperature);
        break;
    case RegisterPosition:
        pointer += offsetof(Telemetry, position);
        size = sizeof(telemetry.position);
        break;
    default:
        break;
    }

    Log.traceln("Send telemetries address %X for size %d", pointer, size);

    fillBuffer(pointer, size);
}

bool I2CSlaveController::fillBuffer(const void* buffer, const size_t size)
{
    if (size > I2C_BUFFER_SIZE)
    {
        Log.warningln("Want to fill I2C slave buffer with %d (>%d)", size, I2C_BUFFER_SIZE);

        memset(txBuffer, 0, I2C_BUFFER_SIZE);
        txBufferSize = 0;

        return false;
    }

    txBufferSize = size;
    memcpy(txBuffer, buffer, size);

    return true;
}

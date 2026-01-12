#include "hal/Sensors/I2CSlaveHal.hpp"

#include <Wire.h>
#include <ArduinoLog.h>

#include "config.h"

bool I2CSlaveHal::doBegin()
{
    uint8_t result = 0;

    auto status = readRegister(RegisterPing, &result, sizeof(result));

    Log.traceln("I2C Slave received : %d", result);

    status &= result == I2C_OK;

    return status;
}

bool I2CSlaveHal::query(Telemetry& telemetry)
{
    Log.traceln("I2C Slave queries");

    if (!readRegister(RegisterTelemetryBattery, &telemetry.battery, sizeof(telemetry.battery)))
    {
        return false;
    }

    if (!readRegister(RegisterTelemetrySolar, &telemetry.solar, sizeof(telemetry.solar)))
    {
        return false;
    }

    if (!readRegister(RegisterTelemetryBox, &telemetry.box, sizeof(telemetry.box)))
    {
        return false;
    }

    if (!readRegister(RegisterTelemetryOutdoor, &telemetry.outdoor.basic, sizeof(telemetry.outdoor.basic)))
    {
        return false;
    }

    return true;
}

bool I2CSlaveHal::queryClock(uint32_t& now)
{
    if (!readRegister(RegisterClock, &now, sizeof(now)))
    {
        return false;
    }

    return true;
}

bool I2CSlaveHal::setRegisterToRead(const I2CSlaveRegisterValue reg)
{
    Log.traceln("I2C Slave query ask for %d", reg);

    Wire.beginTransmission(I2C_SLAVE_ADDRESS);
    Wire.write(reg);
    const auto result = Wire.endTransmission();

    if (result != 0)
    {
        Log.warningln("I2C Slave error set register: %d", reg, result);

        initialized = false;

        return false;
    }

    return true;
}

bool I2CSlaveHal::readRegister(const I2CSlaveRegisterValue reg, void* dest, const size_t size)
{
    if (size > I2C_BUFFER_SIZE)
    {
        Log.warningln("I2C Slave query %d for size %d (> %d) KO", reg, size, I2C_BUFFER_SIZE);

        return false;
    }

    if (!setRegisterToRead(reg))
    {
        return false;
    }

    const size_t sizeToReceive = Wire.requestFrom(I2C_SLAVE_ADDRESS, size);

    Log.traceln("I2C Slave query %d for size %d", reg, size);

    if (sizeToReceive != size)
    {
        Log.warningln("I2C Slave error reading reg %d (should received %d of %d bytes)", reg, sizeToReceive, size);

        initialized = false;

        return false;
    }

    const auto sizeReceived = Wire.readBytes(static_cast<uint8_t*>(dest), size);

    if (sizeReceived != size)
    {
        Log.warningln("I2C Slave error reading reg %d (received %d of %d bytes)", reg, sizeToReceive, size);

        initialized = false;

        return false;
    }

    Log.traceln("I2C Slave query %d for size %d OK", reg, sizeToReceive);

    return true;
}

#include "hal/I2CSlaveHal.hpp"

#include <Wire.h>
#include <ArduinoLog.h>

bool I2CSlaveHal::begin(const uint8_t address)
{
    slaveAddress = address;

    uint8_t result = 0;

    auto initialized = readRegister(RegisterPing, &result, sizeof(result));

    Log.traceln("I2C Slave received : %d", result);

    initialized &= result == I2C_OK;

    return initialized;
}

bool I2CSlaveHal::queryTelemetries(Telemetry& telemetry)
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

    if (!readRegister(RegisterTelemetryOutdoorBasic, &telemetry.outdoor.basic, sizeof(telemetry.outdoor.basic)))
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

void I2CSlaveHal::setRegisterToRead(const I2CSlaveRegisterValue reg) const
{
    Log.traceln("I2C Slave query ask for %d", reg);

    Wire.beginTransmission(slaveAddress);
    Wire.write(reg);
    Wire.endTransmission();
}

bool I2CSlaveHal::readRegister(const I2CSlaveRegisterValue reg, void* dest, const size_t size) const
{
    if (size > I2C_BUFFER_SIZE)
    {
        Log.warningln("I2C Slave query %d for size %d (> %d) KO", reg, size, I2C_BUFFER_SIZE);

        return false;
    }

    setRegisterToRead(reg);

    const size_t sizeToReceive = Wire.requestFrom(slaveAddress, size);

    Log.traceln("I2C Slave query %d for size %d", reg, size);

    if (sizeToReceive != size)
    {
        Log.warningln("I2C Slave error reading reg %u (received %u of %u bytes)", reg, sizeToReceive, size);

        return false;
    }

    Wire.readBytes(static_cast<uint8_t*>(dest), size);

    Log.traceln("I2C Slave query %d for size %d OK", reg, sizeToReceive);

    return true;
}

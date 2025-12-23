#include "hal/I2CSlaveHal.hpp"

#include <Wire.h>
#include <ArduinoLog.h>

bool I2CSlaveHal::begin(const uint8_t address)
{
    if (initialized)
    {
        return true;
    }

    Wire.begin();

    Wire.requestFrom(address, 0);

    initialized = Wire.read() == I2C_OK;

    slaveAddress = address;

    return false;
}

bool I2CSlaveHal::queryTelemetries(Telemetry& telemetry)
{
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
    Wire.beginTransmission(slaveAddress);
    Wire.write(reg);
    Wire.endTransmission();
}

bool I2CSlaveHal::readRegister(const I2CSlaveRegisterValue reg, void* dest, const size_t size)
{
    setRegisterToRead(reg);

    const size_t received = Wire.requestFrom(slaveAddress, size);

    if (received != size)
    {
        Log.warningln("I2C Slave error reading reg %u (received %u of %u bytes)", reg, received, size);

        return false;
    }

    Wire.readBytes(static_cast<uint8_t*>(dest), size);

    return true;
}

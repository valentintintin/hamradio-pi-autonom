#include "hal/I2CSlaveHal.hpp"

bool I2CSlaveHal::begin()
{
    if (initialized)
    {
        return true;
    }

    return false;
}

bool I2CSlaveHal::queryTelemetries(Telemetry& telemetry, uint32_t& now)
{
    return false;
}

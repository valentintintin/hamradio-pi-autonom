#include "hal/Ina3221Hal.hpp"

bool Ina3221Hal::begin()
{
    if (initialized)
    {
        return true;
    }

    Log.infoln("INA3221 init");

    initialized = ina3221.begin();

    if (initialized)
    {
        Log.infoln("INA3221 found");
    }
    else
    {
        Log.infoln("INA3221 init failed");
    }

    return initialized;
}

bool Ina3221Hal::query(Telemetry& telemetry)
{
    Log.infoln("Query INA3221");

    telemetry.battery.voltage = ina3221.getBusVoltage(0);
    telemetry.battery.current = ina3221.getCurrentAmps(0) * 1000;

    telemetry.board.voltage = ina3221.getBusVoltage(1);
    telemetry.board.current = ina3221.getCurrentAmps(1) * 1000;

    telemetry.solar.voltage = ina3221.getBusVoltage(2);
    telemetry.solar.current = ina3221.getCurrentAmps(2) * 1000;

    if (telemetry.board.voltage == NAN)
    {
        Log.warningln("INA3221 in error");

        LedController::getInstance().blink(Error, Sensor);

        initialized = false;

        return false;
    }

    return true;
}

#include "hal/MpptChargerHal.hpp"

#include <ArduinoLog.h>

bool MpptChargerHal::begin()
{
    if (initialized)
    {
        return true;
    }

    Log.infoln("Init Mppt");

    initialized = charger.begin();

    if (initialized)
    {
        Log.infoln("Init Mppt OK");
    }
    else
    {
        Log.warningln("Init Mppt failed");
    }

    return initialized;
}

bool MpptChargerHal::queryTelemetries(TelemetryPower &telemetryBattery, TelemetryPower &telemetrySolar, float &temperature)
{
    Log.infoln("Query Mppt telemetries");

    bool result = begin();

    int16_t tempVariable;
    result &= charger.getIndexedValue(VAL_INT_TEMP, &tempVariable);
    temperature = tempVariable / 10.0f;

    result &= charger.getIndexedValue(VAL_VB, &tempVariable);
    telemetryBattery.voltage = tempVariable / 1000.0f;
    result &= charger.getIndexedValue(VAL_IB, &tempVariable);
    telemetryBattery.current = tempVariable;

    result &= charger.getIndexedValue(VAL_VS, &tempVariable);
    telemetrySolar.voltage = tempVariable / 1000.0f;
    result &= charger.getIndexedValue(VAL_IS, &tempVariable);
    telemetrySolar.current = tempVariable;

    if (result)
    {
        Log.infoln("Query Mppt telemetries OK");
    }
    else
    {
        Log.warningln("Query Mppt telemetries failed");
    }

    return result;
}

bool MpptChargerHal::setWatchdog(const uint16_t powerOff, const uint8_t timeout)
{
    Log.infoln("Feed Mppt watchdog with power off %d and timeout %d", powerOff, timeout);

    auto result = begin();

    if (result)
    {
        result &= charger.setWatchdogPoweroff(powerOff * 1000) && charger.setWatchdogTimeout(timeout);

        Log.infoln("Feed Mppt watchdog OK");
    }
    else
    {
        Log.warningln("Feed Mppt watchdog failed");
    }

    return result;
}

bool MpptChargerHal::setVoltageLimits(const uint16_t powerOff, const uint16_t powerOn)
{
    Log.infoln("Mppt charge set limit voltage power off %d and power on %d", powerOff, powerOn);

    if (powerOn < 12000 || powerOn > 13000)
    {
        Log.warningln("Power on outside range 12000 - 13000 mV");
        return false;
    }

    if (powerOff < 11000 || powerOff > powerOn)
    {
        Log.warningln("Power off outside range 11000 - %d mV", powerOn);
        return false;
    }

    auto result = begin();

    if (result)
    {
        result &= charger.setConfigurationValue(CFG_PWR_ON_TH, powerOn) && charger.setConfigurationValue(CFG_PWR_OFF_TH, powerOff);

        Log.infoln("Mppt charger set limit voltage OK");
    }
    else
    {
        Log.warningln("Mppt charger set limit voltage failed");
    }

    return result;
}

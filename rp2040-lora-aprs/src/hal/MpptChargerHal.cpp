#include "hal/MpptChargerHal.hpp"

#include <ArduinoLog.h>

MpptChargerHal::MpptChargerHal()
{
    i2cSemaphore = xSemaphoreCreateBinary();
    if (i2cSemaphore == nullptr)
    {
        Log.errorln("Failed to create I2C semaphore");
    }

    xSemaphoreGive(i2cSemaphore);
}

bool MpptChargerHal::begin()
{
    if (initialized)
    {
        return true;
    }

    Log.infoln("Init Mppt");

    const auto result = takeSemaphore() && charger.begin();

    initialized = result;

    if (result)
    {
        Log.infoln("Init Mppt OK");
    }
    else
    {
        Log.warningln("Init Mppt failed");
    }

    xSemaphoreGive(i2cSemaphore);

    return result;
}

bool MpptChargerHal::queryTelemetries(TelemetryPower &telemetryBattery, TelemetryPower &telemetrySolar, float &temperature)
{
    Log.infoln("Query Mppt telemetries");

    bool result = begin() && takeSemaphore();

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

    xSemaphoreGive(i2cSemaphore);

    return result;
}

bool MpptChargerHal::feedDog(const uint16_t powerOff, const uint8_t timeout)
{
    Log.infoln("Feed Mppt watchdog with power off %d and timeout %d", powerOff, timeout);

    auto result = begin() && takeSemaphore();

    if (result)
    {
        result &= charger.setWatchdogPoweroff(powerOff) && charger.setWatchdogTimeout(timeout);

        Log.infoln("Feed Mppt watchdog OK");

        xSemaphoreGive(i2cSemaphore);
    }
    else
    {
        Log.warningln("Feed Mppt watchdog failed");
    }

    return result;
}

bool MpptChargerHal::takeSemaphore() const
{
    if (xSemaphoreTake(i2cSemaphore, pdMS_TO_TICKS(2500)) == pdTRUE)
    {
        Log.warningln("Impossible to have I2C semaphore");

        return false;
    }

    return true;
}

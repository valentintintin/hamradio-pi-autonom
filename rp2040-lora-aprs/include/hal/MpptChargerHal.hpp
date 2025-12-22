#pragma once

#include "../../lib/mpptChg/mpptChg.h"
#include "telemetry.h"

class MpptChargerHal
{
public:
    static MpptChargerHal& getInstance()
    {
        static MpptChargerHal instance;
        return instance;
    }

    bool begin();
    bool queryTelemetries(TelemetryPower &telemetryBattery, TelemetryPower &telemetrySolar, float &temperature);
    bool feedDog(uint16_t powerOff = 10, uint8_t timeout = 255);

    const bool isInitialized() const
    {
        return initialized;
    }

private:
    bool initialized = false;
    mpptChg charger;
    QueueHandle_t i2cSemaphore;

    MpptChargerHal();
    bool takeSemaphore() const;
};


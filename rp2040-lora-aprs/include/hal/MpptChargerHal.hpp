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

    bool isInitialized() const
    {
        return initialized;
    }

    bool begin();
    bool queryTelemetries(TelemetryPower &telemetryBattery, TelemetryPower &telemetrySolar, float &temperature);
    bool setWatchdog(uint16_t powerOff = 10, uint8_t timeout = 255);
    bool setVoltageLimits(uint16_t powerOff, uint16_t powerOn);

private:
    bool initialized = false;
    mpptChg charger;
};


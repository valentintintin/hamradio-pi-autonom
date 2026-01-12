#pragma once

#include "SensorHal.hpp"
#include "../../../lib/mpptChg/mpptChg.h"
#include "telemetry.h"

class MpptChargerHal : public SensorHal
{
public:
    static MpptChargerHal& getInstance()
    {
        static MpptChargerHal instance;
        return instance;
    }

    bool query(Telemetry& telemetry) override;

    bool setWatchdog(uint16_t powerOff = 10, uint8_t timeout = 255);
    bool setVoltageLimits(uint16_t powerOff, uint16_t powerOn);
protected:
    bool doBegin() override;
private:
    mpptChg charger;
};


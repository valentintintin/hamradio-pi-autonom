#pragma once
#include "telemetry.h"

#define REG_PING 0x
// On réserve un offset pour "now" juste après la fin de Telemetry
#define REG_NOW_OFFSET REG_PING + sizeof(Telemetry)

class I2CSlaveHal
{
public:
    bool begin();
    bool queryTelemetries(Telemetry& telemetry, uint32_t& now);
private:
    bool initialized = false;
};

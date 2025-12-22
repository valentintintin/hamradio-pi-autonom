#include "controllers/I2CSlaveController.hpp"

#include <Wire.h>
#include <ArduinoLog.h>

#include "controllers/SensorController.hpp"
#include "utils/utils.h"

volatile uint16_t I2CSlaveController::regIndex = 0;

bool I2CSlaveController::begin()
{
    if (!Wire1.setSDA(2)) {
        Log.errorln("I2C Slave init failed, wrong SDA pin");

        return false;
    }

    if (!Wire1.setSCL(3)) {
        Log.errorln("I2C Slave init failed, wrong SCL pin");

        return false;
    }

    Wire1.begin(I2C_ADDR);
    Wire1.onReceive(onReceive);
    Wire1.onRequest(onRequest);

    Log.infoln("I2C Slave init OK");

    return true;
}

void I2CSlaveController::onReceive(const int regIndexWanted)
{
    Log.infoln("I2C Slave, index wanted %d", regIndexWanted);

    if (regIndexWanted >= 1)
    {
        regIndex = Wire1.read();
    }
}

void I2CSlaveController::onRequest()
{
    const auto telemetry = SensorController::getTelemetry();

    const uint8_t* base = (uint8_t*)&telemetry;

    if (regIndex < sizeof(Telemetry))
    {
        Wire.write(base + regIndex, sizeof(telemetry) - regIndex);
    }
    // else if (regIndex == REG_NOW_OFFSET)
    // {
        // uint32_t now = getDateTime().unixtime();
        // Wire.write((uint8_t*)&now, sizeof(now));
    // }
}
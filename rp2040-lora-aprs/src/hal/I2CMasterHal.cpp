#include "hal/I2CMasterHal.hpp"

#include <ArduinoLog.h>

I2CMasterHal::I2CMasterHal()
{
    i2cSemaphore = xSemaphoreCreateBinary();
    if (i2cSemaphore == nullptr)
    {
        Log.errorln("Failed to create I2C semaphore");
    }

    xSemaphoreGive(i2cSemaphore);
}

bool I2CMasterHal::takeSemaphore()
{
    Log.traceln("I2C Master want semaphore");

    if (xSemaphoreTake(getInstance().i2cSemaphore, pdMS_TO_TICKS(2500)) != pdTRUE)
    {
        Log.warningln("Impossible to have I2C master semaphore");

        return false;
    }

    Log.noticeln("I2C Master has got semaphore");

    return true;
}

void I2CMasterHal::releaseSemaphore()
{
    if (xSemaphoreGive(getInstance().i2cSemaphore) == pdTRUE)
    {
        Log.noticeln("I2C Master released");
    }
}

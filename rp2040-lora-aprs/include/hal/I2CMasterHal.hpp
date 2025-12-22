#pragma once

#include <FreeRTOS.h>
#include <queue.h>

class I2CMasterHal
{
public:
    static bool takeSemaphore();
    static void releaseSemaphore();
private:
    QueueHandle_t i2cSemaphore;

    static I2CMasterHal& getInstance()
    {
        static I2CMasterHal instance;
        return instance;
    }

    I2CMasterHal();
};
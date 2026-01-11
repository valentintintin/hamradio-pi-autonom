#pragma once

#include "BaseController.hpp"

#include <queue.h>

#define LED_DELAY 1000
#define LED_MAX_COMMAND 5

enum LedState
{
    LedSuccess = 100,
    LedNormal = 250,
    LedError = 500,
};

enum LedOrigin
{
    LedWatchdog = 1,
    LedFreeRtos,
    LedSettings,
    LedRelay,
    LedI2C,
    LedI2CSlave,
    LedSensor,
    LedMpptCharger,
    LedEEProm,
    LedClock,
    LedRadio,
    LedUserCommand,
};

struct LedCommand
{
    LedState state = LedNormal;
    LedOrigin origin;
};

class LedController : public BaseController
{
public:
    static LedController& getInstance()
    {
        static LedController instance;
        return instance;
    }

    bool begin() override;
    void blink(LedState state, LedOrigin origin) const;

private:
    static void task(void* pvParameters);

    QueueHandle_t queue;

    LedController();
};


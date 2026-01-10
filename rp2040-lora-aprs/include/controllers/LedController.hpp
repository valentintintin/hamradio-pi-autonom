#pragma once

#include "BaseController.hpp"

#include <queue.h>

#define LED_DELAY 1000
#define LED_MAX_COMMAND 5

enum LedState
{
    Success = 100,
    Normal = 250,
    Error = 500,
};

enum LedOrigin
{
    Watchdog = 1,
    FreeRtos,
    Settings,
    Relay,
    I2C,
    I2CSlave,
    Sensor,
    MpptCharger,
    Clock,
    Radio,
    Command,
};

struct LedCommand
{
    LedState state = Normal;
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


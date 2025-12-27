#pragma once

#include "controllers/BaseController.hpp"

#include <timers.h>

#define LED_DELAY 100
#define LED_MAX_DELAY 5000

#define MPPT_CHARGER_POWER_OFF 10
#define MPPT_CHARGER_TIMEOUT 255
#define MPPT_CHARGER_DELAY 60000 // 4 tries for 255 seconds

class WatchdogController : BaseController
{
public:
    static WatchdogController& getInstance()
    {
        static WatchdogController instance;
        return instance;
    }

    bool begin() override;
    bool setMpptWatchdogManagedByUser(uint16_t timeOff, uint8_t timeout = 255);
    bool setMpptWatchdogManagedByTask();

private:
    static void heartbeatAndFeedInternalWatchdog(TimerHandle_t timer);
    static void feedMpptChargerWatchdog(TimerHandle_t timer);

    TimerHandle_t timer;
    TimerHandle_t timerMpptCharger;

    WatchdogController();
};

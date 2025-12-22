#include "controllers/WatchdogController.hpp"
#include "hal/MpptChargerHal.hpp"

#include <ArduinoLog.h>
#include <timers.h>

#include "hal/I2CMasterHal.hpp"

WatchdogController::WatchdogController()
{
    timer = xTimerCreate("heartBeatAndInternal", pdMS_TO_TICKS(LED_MAX_DELAY), pdTRUE, nullptr, heartbeatAndFeedInternalWatchdog);

    if (timer == nullptr)
    {
        Log.errorln("Timer heartBeatAndInternal creation failed");
    }

    timerMpptCharger = xTimerCreate("mpptChargerWatchdog", pdMS_TO_TICKS(MPPT_CHARGER_DELAY), pdTRUE, nullptr, feedMpptChargerWatchdog);

    if (timerMpptCharger == nullptr)
    {
        Log.errorln("Timer mpptChargerWatchdog creation failed");
    }
}

bool WatchdogController::begin()
{
    bool result = false;

    if (timer)
    {
        if (xTimerStart(timer, 0) == pdFAIL)
        {
            Log.errorln("Timer heartBeatAndInternal start failed");
        }
        else
        {
            rp2040.wdt_begin(8300);

            result = true;

            Log.infoln("Timer heartBeatAndInternal started");
        }
    }

    if (timerMpptCharger)
    {
        if (!I2CMasterHal::takeSemaphore())
        {
            Log.warningln("Watchdog mppt can not begin, can not have semaphore");
            // TODO retry
        }
        else
        {
            if (MpptChargerHal::getInstance().begin())
            {
                if (xTimerStart(timerMpptCharger, 0) == pdFAIL)
                {
                    Log.errorln("Timer mpptChargerWatchdog start failed");
                }
                else
                {
                    result &= true;

                    Log.infoln("Timer mpptChargerWatchdog started");
                }
            }

            I2CMasterHal::releaseSemaphore();
        }
    }

    return result;
}

void WatchdogController::heartbeatAndFeedInternalWatchdog(TimerHandle_t timer)
{
    rp2040.wdt_reset();

    digitalWrite(LED_BUILTIN, HIGH);
    vTaskDelay(pdMS_TO_TICKS(LED_DELAY));
    digitalWrite(LED_BUILTIN, LOW);
    vTaskDelay(pdMS_TO_TICKS(LED_DELAY));
}

void WatchdogController::feedMpptChargerWatchdog(TimerHandle_t timer)
{
    if (!I2CMasterHal::takeSemaphore())
    {
        Log.warningln("Mppt watchdog can not feed, can not have semaphore");
        return;
    }

    if (MpptChargerHal::getInstance().isInitialized())
    {
        Log.infoln("Try to feed Mppt watchdog");

        MpptChargerHal::getInstance().feedDog(MPPT_CHARGER_POWER_OFF, MPPT_CHARGER_TIMEOUT);
    }

    I2CMasterHal::releaseSemaphore();
}

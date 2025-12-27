#include "controllers/WatchdogController.hpp"
#include "hal/MpptChargerHal.hpp"

#include <ArduinoLog.h>
#include <timers.h>

#include "SettingsManager.hpp"
#include "hal/I2CMasterHal.hpp"

WatchdogController::WatchdogController()
{
    timer = xTimerCreate("heartBeatAndInternal", pdMS_TO_TICKS(LED_MAX_DELAY), pdTRUE, nullptr, heartbeatAndFeedInternalWatchdog);

    if (timer == nullptr)
    {
        Log.errorln("Timer heartBeatAndInternal creation failed");
    }

    const auto& settingsMpptWatchdog = SettingsManager::getSettings().mpptWatchdog;
    if (settingsMpptWatchdog.enabled)
    {
        timerMpptCharger = xTimerCreate("mpptChargerWatchdog", pdMS_TO_TICKS(settingsMpptWatchdog.intervalFeed), pdTRUE, nullptr, feedMpptChargerWatchdog);

        if (timerMpptCharger == nullptr)
        {
            Log.errorln("Timer mpptChargerWatchdog creation failed");
        }
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
            if (SettingsManager::getSettings().useWatchdog)
            {
                Log.infoln("Use internal watchdog (8300ms)");

                rp2040.wdt_begin(8300);
            }

            result = true;

            Log.infoln("Timer heartBeatAndInternal started");
        }
    }

    if (timerMpptCharger)
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

    return result;
}

void WatchdogController::heartbeatAndFeedInternalWatchdog(TimerHandle_t timer)
{
    if (SettingsManager::getSettings().useWatchdog)
    {
        rp2040.wdt_reset();
    }

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

    Log.infoln("Feed Mppt watchdog");

    const auto& settingsMpptWatchdog = SettingsManager::getSettings().mpptWatchdog;

    MpptChargerHal::getInstance().feedDog(settingsMpptWatchdog.timeOff, settingsMpptWatchdog.timeout);

    I2CMasterHal::releaseSemaphore();
}

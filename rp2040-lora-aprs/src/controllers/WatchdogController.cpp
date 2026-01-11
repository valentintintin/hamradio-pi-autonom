#include "controllers/WatchdogController.hpp"

#include <ArduinoLog.h>
#include <timers.h>

#include "SettingsManager.hpp"
#include "hal/MpptChargerHal.hpp"
#include "controllers/LedController.hpp"
#include "hal/I2CMasterHal.hpp"

WatchdogController::WatchdogController()
{
    if (SettingsManager::getSettings().useWatchdog)
    {
        timer = xTimerCreate("watchdogInternal", pdMS_TO_TICKS(WATCHDOG_INTERNAL_MAX_DELAY), pdTRUE, nullptr, feedInternalWatchdog);

        if (timer == nullptr)
        {
            Log.errorln("Timer heartBeatAndInternal creation failed");

            LedController::getInstance().blink(Error, FreeRtos);
        }
        else
        {
            Log.infoln("Use internal watchdog (8300ms)");

            rp2040.wdt_begin(8300);
        }
    }

    const auto& settingsMpptWatchdog = SettingsManager::getSettings().mppt.watchdog;
    if (settingsMpptWatchdog.enabled)
    {
        timerMpptCharger = xTimerCreate("mpptChargerWatchdog", pdMS_TO_TICKS(settingsMpptWatchdog.intervalFeed * 1000), pdTRUE, nullptr, feedMpptChargerWatchdog);

        if (timerMpptCharger == nullptr)
        {
            Log.errorln("Timer mpptChargerWatchdog creation failed");

            LedController::getInstance().blink(Error, FreeRtos);
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
            Log.errorln("Timer watchdogInternal start failed");

            LedController::getInstance().blink(Error, FreeRtos);
        }
        else
        {
            result = true;

            Log.infoln("Timer watchdogInternal started");
        }
    }

    if (timerMpptCharger)
    {
        if (xTimerStart(timerMpptCharger, 0) == pdFAIL)
        {
            Log.errorln("Timer mpptChargerWatchdog start failed");

            LedController::getInstance().blink(Error, FreeRtos);
        }
        else
        {
            result &= true;

            Log.infoln("Timer mpptChargerWatchdog started");
            feedMpptChargerWatchdog(timerMpptCharger);
        }
    }

    return result;
}

bool WatchdogController::setMpptWatchdogManagedByUser(const uint16_t timeOff, const uint8_t timeout)
{
    if (timerMpptCharger == nullptr)
    {
        Log.errorln("Timer mpptChargerWatchdog does not exist");

        return false;
    }

    if (xTimerStop(timerMpptCharger, 0) == pdFAIL)
    {
        Log.errorln("Timer mpptChargerWatchdog stop failed");

        LedController::getInstance().blink(Error, FreeRtos);

        return false;
    }

    Log.infoln("Timer mpptChargerWatchdog stopped");

    if (!MpptChargerHal::getInstance().setWatchdog(timeOff, timeout))
    {
        Log.errorln("Set Mppt watchdog to user values failed");

        LedController::getInstance().blink(Error, MpptCharger);

        return false;
    }

    return true;
}

bool WatchdogController::setMpptWatchdogManagedByTask()
{
    if (timerMpptCharger == nullptr)
    {
        Log.errorln("Timer mpptChargerWatchdog does not exist");

        return false;
    }

    if (xTimerStart(timerMpptCharger, 0) == pdFAIL)
    {
        Log.errorln("Timer mpptChargerWatchdog start failed");

        LedController::getInstance().blink(Error, FreeRtos);

        return false;
    }

    Log.infoln("Timer mpptChargerWatchdog started");

    feedMpptChargerWatchdog(timerMpptCharger);

    return true;
}

void WatchdogController::feedInternalWatchdog(TimerHandle_t timer)
{
    rp2040.wdt_reset();

    Log.traceln("Feed dog");

    LedController::getInstance().blink(Normal, Watchdog);
}

void WatchdogController::feedMpptChargerWatchdog(TimerHandle_t timer)
{
    if (!I2CMasterHal::takeSemaphore())
    {
        Log.warningln("Mppt watchdog can not feed, can not have semaphore");

        LedController::getInstance().blink(Error, I2C);

        return;
    }

    Log.infoln("Feed Mppt watchdog");

    const auto& settingsMpptWatchdog = SettingsManager::getSettings().mppt.watchdog;

    if (!MpptChargerHal::getInstance().setWatchdog(settingsMpptWatchdog.timeOff, settingsMpptWatchdog.timeout))
    {
        Log.warningln("Feed Mppt watchdog KO");

        LedController::getInstance().blink(Error, MpptCharger);
    }

    I2CMasterHal::releaseSemaphore();
}

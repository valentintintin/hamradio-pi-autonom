#include "controllers/CommandController.hpp"

#include "utils/utils.h"
#include "utils/rp2040.h"

#include <timers.h>

#include "SettingsManager.hpp"
#include "controllers/LedController.hpp"
#include "controllers/SensorController.hpp"
#include "controllers/WatchdogController.hpp"
#include "hal/I2CMasterHal.hpp"

bool CommandController::begin()
{
    return true;
}

bool CommandController::processCommand(const char* command)
{
    memset(response, 0, MAX_RESPONSE_LENGTH);

    if (memcmp(command, "get ", 4) == 0)
    {
        const char* configKey = &command[4];
        return SettingsManager::getInstance().getSettingFromString(configKey, response, MAX_RESPONSE_LENGTH);
    }

    if (memcmp(command, "set ", 4) == 0)
    {
        const char* configKey = &command[4];

        const char* value = strchr(configKey, ' ');
        if (value != nullptr && strlen(value) >= 2) // espace + valeur à minima 1 caractère
        {
            return SettingsManager::getInstance().setSettingFromString(configKey, value + 1);
        }

        return false;
    }

    if (memcmp(command, "gpio ", 5) == 0)
    {
        doGpioCommand(&command[5]);
    }

    if (memcmp(command, "dfu", 3) == 0)
    {
        return doDfuCommand();
    }

    if (memcmp(command, "reboot", 6) == 0)
    {
        return doRebootCommand();
    }

    if (memcmp(command, "uptime", 6) == 0)
    {
        return doUptimeCommand();
    }

    if (memcmp(command, "resetReason", 11) == 0)
    {
        return doResetReasonCommand();
    }

    if (memcmp(command, "telem", 5) == 0)
    {
        return doTelemetriesCommand();
    }

    if (memcmp(command, "ping", 4) == 0)
    {
        return doPingCommand();
    }

    if (memcmp(command, "mppt.wdt ", 9) == 0)
    {
        return doMpptWatchdogUserCommand(&command[9]);
    }

    if (memcmp(command, "mppt.voltLimit ", 15) == 0)
    {
        return doMpptVoltageLimitsCommand(&command[15]);
    }

    LedController::getInstance().blink(Error, Command);

    return false;
}

const char* CommandController::getResponse() const
{
    return response;
}

bool CommandController::doRebootCommand()
{
    const TimerHandle_t timer = xTimerCreate("timerReboot", pdMS_TO_TICKS(5000), pdFALSE, nullptr, rebootTask);

    if (timer && xTimerStart(timer, 0) == pdPASS)
    {
        strncpy(response, "Reboot in 5s !", MAX_RESPONSE_LENGTH);
    }
    else
    {
        strncpy(response, "Reboot !", MAX_RESPONSE_LENGTH);
        rebootTask(timer);
    }

    LedController::getInstance().blink(Success, Command);

    return true;
}

bool CommandController::doDfuCommand()
{
    const TimerHandle_t timer = xTimerCreate("timerDfu", pdMS_TO_TICKS(5000), pdFALSE, nullptr, dfuTask);

    WatchdogController::getInstance().setMpptWatchdogManagedByUser(5);

    if (timer && xTimerStart(timer, 0) == pdPASS)
    {
        strncpy(response, "DFU in 5s !", MAX_RESPONSE_LENGTH);
    }
    else
    {
        strncpy(response, "DFU !", MAX_RESPONSE_LENGTH);
        dfuTask(timer);
    }

    LedController::getInstance().blink(Success, Command);

    return true;
}

bool CommandController::doGpioCommand(const char *command)
{
    uint8_t id = 255;
    bool state = false;

    if (memcmp(command, "on ", 3) == 0)
    {
        id = strtol(&command[3], nullptr, 10);
        state = true;
    }

    if (memcmp(command, "off ", 4) == 0)
    {
        id = strtol(&command[4], nullptr, 10);
        state = false;
    }

    if (RelayController::getInstance().changeState(id, state))
    {
        snprintf(response, MAX_RESPONSE_LENGTH, "OK. GPIO %d is %d", id, state);

        LedController::getInstance().blink(Success, Command);

        return true;
    }

    LedController::getInstance().blink(Error, Command);

    snprintf(response, MAX_RESPONSE_LENGTH, "KO");
    return false;
}

bool CommandController::doResetReasonCommand()
{
    snprintf(response, MAX_RESPONSE_LENGTH, "Reset reason: %d", rp2040.getResetReason());

    LedController::getInstance().blink(Success, Command);

    return true;
}

bool CommandController::doUptimeCommand()
{
    snprintf(response, MAX_RESPONSE_LENGTH, "%lu seconds", millis() / 1000);

    LedController::getInstance().blink(Success, Command);

    return true;
}

bool CommandController::doTelemetriesCommand()
{
    if (SensorController::getInstance().queryTelemetries())
    {
        strncpy(response, "OK", MAX_RESPONSE_LENGTH);

        LedController::getInstance().blink(Error, Command);

        return true;
    }

    strncpy(response, "KO", MAX_RESPONSE_LENGTH);

    LedController::getInstance().blink(Success, Command);

    return false;
}

bool CommandController::doPingCommand()
{
    char dateString[64];
    getDateTimeStringFromEpoch(getDateTime().unixtime(), dateString, 64);
    snprintf(response, MAX_RESPONSE_LENGTH, "Pong!\n%s", dateString);

    LedController::getInstance().blink(Success, Command);

    return true;
}

bool CommandController::doMpptVoltageLimitsCommand(const char* command)
{
    const char* space = strchr(command, ' ');
    if (!space)
    {
        strncpy(response, "KO args: VOff VOn -> mV", MAX_RESPONSE_LENGTH);

        LedController::getInstance().blink(Error, Command);

        return false;
    }

    char* endPointer = nullptr;

    const auto powerOff = static_cast<uint16_t>(strtol(command, &endPointer, 10));
    const auto powerOn = static_cast<uint16_t>(strtol(space + 1, &endPointer, 10));

    if (!I2CMasterHal::takeSemaphore())
    {
        strncpy(response, "KO semaphore", MAX_RESPONSE_LENGTH);

        LedController::getInstance().blink(Error, I2C);

        return false;
    }

    if (MpptChargerHal::getInstance().setVoltageLimits(powerOff, powerOn))
    {
        snprintf(response, MAX_RESPONSE_LENGTH, "OK. VOff %dmV, VOn %dmV", powerOff, powerOn);

        LedController::getInstance().blink(Success, Command);

        return true;
    }

    I2CMasterHal::releaseSemaphore();

    snprintf(response, MAX_RESPONSE_LENGTH, "KO. VOff %dmV, VOn %dmV", powerOff, powerOn);

    LedController::getInstance().blink(Error, Command);

    return false;
}

bool CommandController::doMpptWatchdogUserCommand(const char *command)
{
    const char* space = strchr(command, ' ');
    if (!space)
    {
        strncpy(response, "KO args: TOff TOn -> sec", MAX_RESPONSE_LENGTH);

        LedController::getInstance().blink(Error, Command);

        return false;
    }

    char* endPointer = nullptr;

    const auto timeOff = static_cast<uint16_t>(strtol(command, &endPointer, 10));
    const auto timeout = static_cast<uint8_t>(strtol(space + 1, &endPointer, 10));

    if (WatchdogController::getInstance().setMpptWatchdogManagedByUser(timeOff, timeout))
    {
        snprintf(response, MAX_RESPONSE_LENGTH, "OK. TOff %ds, TOn %ds", timeOff, timeout);

        LedController::getInstance().blink(Success, Command);

        return true;
    }

    snprintf(response, MAX_RESPONSE_LENGTH, "KO. TOff %ds, TOn %ds", timeOff, timeout);

    LedController::getInstance().blink(Error, Command);

    return false;
}

#include "controllers/CommandController.hpp"

#include "utils/utils.h"
#include "utils/rp2040.h"

#include <timers.h>

#include "SettingsManager.hpp"

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

    if (memcmp(command, "gpio ", 5) == 0)
    {
        if (memcmp(&command[5], "on ", 3) == 0)
        {
            const auto pin = strtol(&command[5 + 3], nullptr, 10);
            return doGpioOutput(pin, true);
        }

        if (memcmp(&command[5], "off ", 4) == 0)
        {
            const auto pin = strtol(&command[5 + 4], nullptr, 10);
            return doGpioOutput(pin, false);
        }
    }

    return false;
}

const char* CommandController::getResponse() const
{
    return response;
}

bool CommandController::doRebootOutput()
{
    TimerHandle_t timer = xTimerCreate("timerReboot", pdMS_TO_TICKS(10000), pdFALSE, nullptr, rebootTask);

    if (xTimerStart(timer, 0) == pdPASS)
    {
        strncpy(response, "Reboot in 10s !", MAX_RESPONSE_LENGTH);
    }
    else
    {
        strncpy(response, "Reboot !", MAX_RESPONSE_LENGTH);
        rebootTask(timer);
    }

    return true;
}

bool CommandController::doDfuOutput()
{
    TimerHandle_t timer = xTimerCreate("timerDfu", pdMS_TO_TICKS(10000), pdFALSE, nullptr, dfuTask);

    if (xTimerStart(timer, 0) == pdPASS)
    {
        strncpy(response, "DFU in 10s !", MAX_RESPONSE_LENGTH);
    }
    else
    {
        strncpy(response, "DFU !", MAX_RESPONSE_LENGTH);
        dfuTask(timer);
    }

    return true;
}

bool CommandController::doGpioOutput(const uint8_t id, const bool state)
{
    if (RelayController::getInstance().changeState(id, state))
    {
        snprintf(response, MAX_RESPONSE_LENGTH, "OK. GPIO %d is %d", id, state);

        return true;
    }

    snprintf(response, MAX_RESPONSE_LENGTH, "KO");
    return false;
}

bool CommandController::doResetReasonOutput()
{
    snprintf(response, MAX_RESPONSE_LENGTH, "Reset reason: %d", rp2040.getResetReason());

    return true;
}

bool CommandController::doUptimeOutput()
{
    snprintf(response, MAX_RESPONSE_LENGTH, "%lu seconds", millis() / 1000);

    return true;
}

bool CommandController::doPingOutput()
{
    char dateString[64];
    getDateTimeStringFromEpoch(getDateTime().unixtime(), dateString, 64);
    snprintf(response, MAX_RESPONSE_LENGTH, "Pong!\n%s", dateString);

    return true;
}
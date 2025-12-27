#pragma once

#include "controllers/RelayController.hpp"

#define MAX_RESPONSE_LENGTH 64

class CommandController : BaseController
{
public:
    static CommandController& getInstance()
    {
        static CommandController instance;
        return instance;
    }

    bool begin() override;

    bool processCommand(const char* command);
    const char* getResponse() const;

private:
    char response[MAX_RESPONSE_LENGTH]{};

    bool doRebootCommand();
    bool doDfuCommand();
    bool doGpioCommand(const char *command);
    bool doResetReasonCommand();
    bool doUptimeCommand();
    bool doTelemetriesCommand();
    bool doPingCommand();
    bool doMpptVoltageLimitsCommand(const char *command);
    bool doMpptWatchdogUserCommand(const char *command);
};

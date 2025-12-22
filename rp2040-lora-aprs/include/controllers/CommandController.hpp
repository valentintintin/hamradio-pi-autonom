#pragma once

#include "controllers/RelayController.hpp"

#define MAX_RESPONSE_LENGTH 128

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

    bool doRebootOutput();
    bool doDfuOutput();
    bool doGpioOutput(uint8_t id, bool state);
    bool doResetReasonOutput();
    bool doUptimeOutput();
    bool doPingOutput();
};

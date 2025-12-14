#pragma once

#include <CommandParser.h>

#include "controllers/RelayController.hpp"

//                  COMMANDS, COMMAND_ARGS, COMMAND_NAME_LENGTH, COMMAND_ARG_SIZE, COMMAND_HLP_LENGTH, RESPONSE_SIZE
typedef CommandParser<16,       2,              16,                 200 ,             0,               128> MyCommandParser; // Response size > 200 bug

class CommandController {
public:
    explicit CommandController(const RelayController *relayController);
    bool processCommand(const char *command);
    const char* getResponse() const;
private:
    char response[MyCommandParser::MAX_RESPONSE_SIZE]{};
    MyCommandParser parser;

    static const RelayController *relayController;

    static void doRebootOutput(MyCommandParser::Argument *args, char *response);
    static void doDfuOutput(MyCommandParser::Argument *args, char *response);
    static void doGpioOutput(MyCommandParser::Argument *args, char *response);
    static void doResetReasonOutput(MyCommandParser::Argument *args, char *response);
    static void doUptimeOutput(MyCommandParser::Argument *args, char *response);
    static void doPingOutput(MyCommandParser::Argument *args, char *response);
};
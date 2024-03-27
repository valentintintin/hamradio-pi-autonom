#ifndef CUBECELL_MONITORING_COMMAND_H
#define CUBECELL_MONITORING_COMMAND_H

#include <CommandParser.h>

typedef CommandParser<16, 2, 16, 250, 0, 32> MyCommandParser;

class System;

class Command {
public:
    explicit Command(System *system);

    bool processCommand(const char *command);
private:
    static System *system;

    MyCommandParser parser;
    char response[MyCommandParser::MAX_RESPONSE_SIZE]{};

    static void doWifi(MyCommandParser::Argument *args, char *response);

    static void doNpr(MyCommandParser::Argument *args, char *response);

    static void doPosition(MyCommandParser::Argument *args, char *response);

    static void doTelemetry(MyCommandParser::Argument *args, char *response);

    static void doTelemetryParams(MyCommandParser::Argument *args, char *response);

    static void doWatchdog(MyCommandParser::Argument *args, char *response);

    static void doMpptPower(MyCommandParser::Argument *args, char *response);

    static void doLora(MyCommandParser::Argument *args, char *response);

    static void doReset(MyCommandParser::Argument *args, char *response);

    static void doSetEeprom(MyCommandParser::Argument *args, char *response);

    static void doSetTime(MyCommandParser::Argument *args, char *response);

    static void doSleep(MyCommandParser::Argument *args, char *response);

    static void doGetJson(MyCommandParser::Argument *args, char *response);
};

#endif //CUBECELL_MONITORING_COMMAND_H

#ifndef CUBECELL_MONITORING_COMMAND_H
#define CUBECELL_MONITORING_COMMAND_H

#include <CommandParser.h>

typedef CommandParser<> MyCommandParser;

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

    static void doTelemetry(MyCommandParser::Argument *args, char *response);

    static void doTelemetryParams(MyCommandParser::Argument *args, char *response);

    static void doWatchdog(MyCommandParser::Argument *args, char *response);

    static void doMpptPower(MyCommandParser::Argument *args, char *response);

    static void doLora(MyCommandParser::Argument *args, char *response);

//    static void doSetTime(MyCommandParser::Argument *args, char *response);
};

#endif //CUBECELL_MONITORING_COMMAND_H

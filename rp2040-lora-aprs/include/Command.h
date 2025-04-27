#ifndef MONITORING_COMMAND_H
#define MONITORING_COMMAND_H

#include <CommandParser.h>
#include <GpioPin.h>
#include <Settings.h>

class System;

//                  COMMANDS, COMMAND_ARGS, COMMAND_NAME_LENGTH, COMMAND_ARG_SIZE, COMMAND_HLP_LENGTH, RESPONSE_SIZE
typedef CommandParser<32,       2,              16,                     200,                0,              200> MyCommandParser;

class Command {
public:
    char response[MyCommandParser::MAX_RESPONSE_SIZE + 1]{};

    explicit Command(System *system);
    bool processCommand(Stream* stream, const char *command);
private:
    static System *system;
    static SettingsAprsCallsignHeard *sortedAprsHeard[APRS_CALLSIGNS_HEARD_NUMBER];

    MyCommandParser parser;

    static void doPosition(MyCommandParser::Argument *args, char *response);
    static void doTelemetry(MyCommandParser::Argument *args, char *response);
    static void doTelemetryParams(MyCommandParser::Argument *args, char *response);
    static void doStatus(MyCommandParser::Argument *args, char *response);
    static void doLora(MyCommandParser::Argument *args, char *response);
    static void doReboot(MyCommandParser::Argument *args, char *response);
    static void doDfu(MyCommandParser::Argument *args, char *response);
    static void doPrintJson(MyCommandParser::Argument *args, char *response);
    static void doPing(MyCommandParser::Argument *args, char *response);
    static void doGpioOutput(MyCommandParser::Argument *args, char *response);
    static void doSetSetting(MyCommandParser::Argument *args, char *response);
    static void doGetSetting(MyCommandParser::Argument *args, char *response);
    static void doMpptWatchdog(MyCommandParser::Argument *args, char *response);
    static void doMeshtasticAprs(MyCommandParser::Argument *args, char *response);
    static void doLinuxAprs(MyCommandParser::Argument *args, char *response);
    static void doGetBoxInfo(MyCommandParser::Argument *args, char *response);
    static void doGetError(MyCommandParser::Argument *args, char *response);
    static void doSetLora(MyCommandParser::Argument *args, char *response);
    static void doSleepLinux(MyCommandParser::Argument *args, char *response);
    static void doCommandMeshtastic(MyCommandParser::Argument *args, char *response);
    static void doGetCommandResponseFromMeshtastic(MyCommandParser::Argument *args, char *response);

    static void doAprsQueryHelp(MyCommandParser::Argument *args, char *response);
    static void doAprsHeardWithoutDigi(MyCommandParser::Argument *args, char *response);
    static void doAprsHeard(MyCommandParser::Argument *args, char *response);
    static void doAprsHeardSomeone(MyCommandParser::Argument *args, char *response);
    static void doAbout(MyCommandParser::Argument *args, char *response);
    static void doAprsPing(MyCommandParser::Argument *args, char *response);

    static void sortAprsHeard();
    static int compareAprsHeardTimeDescending(const void *a, const void *b);
    static bool changeGpio(const char *what, uint16_t state);
};

#endif //MONITORING_COMMAND_H

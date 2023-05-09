#include <pgmspace.h>
#include "ArduinoLog.h"

#include "Command.h"
#include "System.h"

System* Command::system;

Command::Command(System *system) {
    Command::system = system;

    parser.registerCommand(PSTR("wifi"), PSTR("s"), doWifi);
    parser.registerCommand(PSTR("npr"), PSTR("s"), doNpr);
    parser.registerCommand(PSTR("telem"), PSTR(""), doTelemetry);
    parser.registerCommand(PSTR("dog"), PSTR("u"), doWatchdog);
    parser.registerCommand(PSTR("lora"), PSTR("s"), doLora);
}

bool Command::processCommand(const char *command) {
    if (strlen(command) < 3) {
        Log.traceln(F("[COMMAND] Command received length %d : %s"), strlen(command), command);
        return false;
    }

    Log.traceln(F("[COMMAND] Process : %s"), command);

    if (!parser.processCommand(command, response)) {
        return false;
    }

    Log.infoln(F("[COMMAND] %s = %s"), command, response);

    return true;
}

void Command::doWifi(MyCommandParser::Argument *args, char *response) {
    bool state = strstr_P(args[0].asString, PSTR("on"));

    system->gpio.setWifi(state);

    sprintf_P(response, PSTR("OK"));
}

void Command::doNpr(MyCommandParser::Argument *args, char *response) {
    bool state = strstr_P(args[0].asString, PSTR("on"));

    system->gpio.setNpr(state);

    sprintf_P(response, PSTR("OK"));
}

void Command::doTelemetry(MyCommandParser::Argument *args, char *response) {
    system->forceSendTelemetry = true;

    sprintf_P(response, PSTR("OK"));
}

void Command::doWatchdog(MyCommandParser::Argument *args, char *response) {
    uint64_t watchdog = args[0].asUInt64;

    bool ok = system->mpptMonitor.setWatchdog(watchdog);

    sprintf_P(response, ok ? PSTR("OK") : PSTR("KO"));
}

void Command::doLora(MyCommandParser::Argument *args, char *response) {
    char *message = args[0].asString;

    system->communication->sendMessage(PSTR(APRS_DESTINATION), message);

    sprintf_P(response, PSTR("OK"));
}

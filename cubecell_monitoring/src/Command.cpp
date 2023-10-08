#include <pgmspace.h>
#include "ArduinoLog.h"

#include "Command.h"
#include "System.h"

System* Command::system;

Command::Command(System *system) {
    Command::system = system;

    parser.registerCommand(PSTR("wifi"), PSTR("u"), doWifi);
    parser.registerCommand(PSTR("npr"), PSTR("u"), doNpr);
    parser.registerCommand(PSTR("telem"), PSTR(""), doTelemetry);
    parser.registerCommand(PSTR("telemParams"), PSTR(""), doTelemetry);
    parser.registerCommand(PSTR("dog"), PSTR("u"), doWatchdog);
    parser.registerCommand(PSTR("pow"), PSTR("uu"), doMpptPower);
    parser.registerCommand(PSTR("lora"), PSTR("s"), doLora);
//    parser.registerCommand(PSTR("time"), PSTR("u"), doSetTime);
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
    bool state = args[0].asInt64 > 0;

    system->gpio.setWifi(state);

    sprintf_P(response, PSTR("OK with state %d"), state);
}

void Command::doNpr(MyCommandParser::Argument *args, char *response) {
    bool state = args[0].asInt64 > 0;

    system->gpio.setNpr(state);

    sprintf_P(response, PSTR("OK with state %d"), state);
}

void Command::doTelemetry(MyCommandParser::Argument *args, char *response) {
    system->forceSendTelemetry = true;

    sprintf_P(response, PSTR("OK"));
}

void Command::doTelemetryParams(MyCommandParser::Argument *args, char *response) {
    system->communication->shouldSendTelemetryParams = true;

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

void Command::doMpptPower(MyCommandParser::Argument *args, char *response) {
    uint64_t powerOnVoltage = args[0].asUInt64;
    uint64_t powerOffVoltage = args[1].asUInt64;

    bool ok = system->mpptMonitor.setPowerOnOff(powerOnVoltage, powerOffVoltage);

    sprintf_P(response, ok ? PSTR("OK") : PSTR("KO"));
}

//void Command::doSetTime(MyCommandParser::Argument *args, char *response) {
//    uint64_t epoch = args[0].asUInt64;
//
//    system->RTC.setEpoch((long long) epoch, true);
//    system->setTimeFromRTcToInternalRtc(epoch);
//
//    System::nowToString(response);
//}


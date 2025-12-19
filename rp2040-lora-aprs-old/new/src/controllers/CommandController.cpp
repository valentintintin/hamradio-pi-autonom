#include "controllers/CommandController.hpp"

#include "utils/utils.h"
#include "utils/rp2040.h"

#include <FreeRTOS.h>
#include <timers.h>

const RelayController* CommandController::relayController;

CommandController::CommandController(const RelayController *relayController) {
    CommandController::relayController = relayController;

    parser.registerCommand("gpio", "uu", &doGpioOutput);
    parser.registerCommand("reboot", "", &doRebootOutput);
    parser.registerCommand("dfu", "", &doDfuOutput);
    parser.registerCommand("uptime", "", &doUptimeOutput);
    parser.registerCommand("resetReason", "", &doResetReasonOutput);
    parser.registerCommand("ping", "", &doPingOutput);
}

bool CommandController::processCommand(const char *command) {
    return parser.processCommand(command, response);
}

const char* CommandController::getResponse() const {
    return response;
}

void CommandController::doRebootOutput(MyCommandParser::Argument *args, char *response) {
    const TimerHandle_t timer = xTimerCreate("timerReboot", pdMS_TO_TICKS(10000), pdTRUE, nullptr, rebootTask);

    if (xTimerStart(timer, 0) == pdPASS) {
        strncpy(response, "Reboot in 10s !", MyCommandParser::MAX_RESPONSE_SIZE);
    } else {
        strncpy(response, "Reboot !", MyCommandParser::MAX_RESPONSE_SIZE);
        rebootTask(timer);
    }
}

void CommandController::doDfuOutput(MyCommandParser::Argument *args, char *response) {
    const TimerHandle_t timer = xTimerCreate("timerDfu", pdMS_TO_TICKS(10000), pdTRUE, nullptr, dfuTask);

    if (xTimerStart(timer, 0) == pdPASS) {
        strncpy(response, "DFU in 10s !", MyCommandParser::MAX_RESPONSE_SIZE);
    } else {
        strncpy(response, "DFU !", MyCommandParser::MAX_RESPONSE_SIZE);
        dfuTask(timer);
    }
}

void CommandController::doGpioOutput(MyCommandParser::Argument *args, char *response) {
    const auto id = static_cast<uint8_t>(args[0].asUInt64);
    const auto state = args[1].asUInt64 == 1;

    if (relayController->changeState(id, state)) {
        snprintf(response, MyCommandParser::MAX_RESPONSE_SIZE, "OK. GPIO %d is %d", id, state);
    } else {
        snprintf(response, MyCommandParser::MAX_RESPONSE_SIZE, "KO");
    }
}

void CommandController::doResetReasonOutput(MyCommandParser::Argument *args, char *response) {
    snprintf(response, MyCommandParser::MAX_RESPONSE_SIZE, "Reset reason: %d", rp2040.getResetReason());
}

void CommandController::doUptimeOutput(MyCommandParser::Argument *args, char *response) {
    snprintf(response, MyCommandParser::MAX_RESPONSE_SIZE, "%lu seconds", millis() / 1000);
}

void CommandController::doPingOutput(MyCommandParser::Argument *args, char *response) {
    char dateString[64];
    getDateTimeStringFromEpoch(getDateTime().unixtime(), dateString, 64);
    snprintf(response, MyCommandParser::MAX_RESPONSE_SIZE, "Pong!\n%s", dateString);
}
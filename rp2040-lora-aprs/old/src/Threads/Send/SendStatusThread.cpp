#include "Threads/Send/SendStatusThread.h"
#include "ArduinoLog.h"
#include "System.h"

SendStatusThread::SendStatusThread(System *system) : SendThread(system, system->settings.aprs.intervalStatus, PSTR("SEND_STATUS"), system->settings.aprs.statusEnabled) {
}

bool SendStatusThread::runOnce() {
    return system->communication.sendStatus(system->settings.aprs.status);
}
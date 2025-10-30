#include "Threads/Send/SendPositionThread.h"
#include "System.h"
#include "ArduinoLog.h"

SendPositionThread::SendPositionThread(System *system) : SendThread(system, system->settings.aprs.intervalPositionWeather, PSTR("SEND_POSITION"), system->settings.aprs.positionWeatherEnabled) {
}

bool SendPositionThread::runOnce() {
    return system->communication.sendPosition(system->settings.aprs.positionComment);
}

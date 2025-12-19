#include "Threads/BlinkerThread.h"
#include "ArduinoLog.h"
#include "config.h"

BlinkerThread::BlinkerThread(System *system, GpioPin *gpio) : MyThread(system, INTERVAL_BLINKER, PSTR("BLINKER"), true), gpio(gpio) {
}

bool BlinkerThread::runOnce() {
    gpio->setState(HIGH);
    delay(1);
    gpio->setState(LOW);
    delay(1);

    return true;
}
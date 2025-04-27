#include "Threads/Watchdog/WatchdogMasterPinThread.h"

#include "ArduinoLog.h"
#include "System.h"
#include "utils.h"

WatchdogMasterPinThread::WatchdogMasterPinThread(System *system, const char* name, GpioPin *gpio, const unsigned long intervalCheck, const bool enabled):
WatchdogThread(system, intervalCheck, PSTR("WATCHDOG_PIN_"), enabled), gpio(gpio) {
    ThreadName.concat(gpio->getPin());
    ThreadName.concat('_');
    ThreadName.concat(name);
}


bool WatchdogMasterPinThread::init() {
    WatchdogThread::init();

    gpio->setState(true);

    return true;
}

bool WatchdogMasterPinThread::runOnce() {
    if (wantToSleep) { // User ask to sleep
        if (gpio->getState()) { // Currently running
            gpio->setState(LOW); // Shutdown
            timerSleep.restart(); // Start timer
        } else if (timerSleep.hasExpired()) { // Not running and timer expired
            gpio->setState(HIGH); // Turn on
            wantToSleep  = false; // Reset state
        }

        return true;
    }

    if (isFed()) {
        Log.traceln(F("[%S] Dog fed"), ThreadName.c_str());
        return true;
    }

    if (!gpio->getState()) {
        Log.warningln(F("[%S] Dog not fed but pin low"), ThreadName.c_str());
        return true;
    }

    Log.warningln(F("[%S] Dog not fed so toggle pin"), ThreadName.c_str());

    gpio->setState(LOW);
    delayWdt(TIME_WAIT_TOGGLE_WATCHDOG_MASTER);
    gpio->setState(HIGH);

    return true;
}

bool WatchdogMasterPinThread::feed() {
    Log.infoln(F("[%S] Feed dog"), ThreadName.c_str());
    lastFed = millis();
    return true;
}

void WatchdogMasterPinThread::sleep(const uint64_t millis) {
    Log.infoln(F("[%s] Sleep for %ums at next internal (%u)"), ThreadName.c_str(), millis, interval);
    timerSleep.setInterval(millis);
    wantToSleep = true;
    enabled = true;
}
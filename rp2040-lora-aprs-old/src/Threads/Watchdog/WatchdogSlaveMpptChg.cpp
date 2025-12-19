#include "Threads/Watchdog/WatchdogSlaveMpptChgThread.h"

#include "ArduinoLog.h"
#include "System.h"

WatchdogSlaveMpptChgThread::WatchdogSlaveMpptChgThread(System *system) :
WatchdogThread(system, system->settings.mpptWatchdog.intervalFeed, PSTR("WATCHDOG_MPPTCHG")) {
    charger = &system->mpptChgCharger;
    enabled = system->settings.mpptWatchdog.enabled;
}

bool WatchdogSlaveMpptChgThread::init() {
    if (charger->begin()
        && charger->setWatchdogPoweroff(system->settings.mpptWatchdog.timeOff)
        && charger->setWatchdogTimeout(system->settings.mpptWatchdog.timeout)
        && charger->setWatchdogEnable(system->settings.mpptWatchdog.enabled)) {
        Log.infoln(F("[WATCHDOG_MPPTCHG] Initiated to %T with power off %d and timeout %d"), system->settings.mpptWatchdog.enabled, system->settings.mpptWatchdog.timeOff, system->settings.mpptWatchdog.timeout);
        return true;
    }

    return false;
}

bool WatchdogSlaveMpptChgThread::runOnce() {
    return managedByUser || feed();
}

bool WatchdogSlaveMpptChgThread::feed() {
    if (!_initiated && !begin()) {
        Log.errorln(F("[WATCHDOG_MPPTCHG] Fail to feed dog, not initiated"));
        return false;
    }

    if (!charger->setWatchdogTimeout(system->settings.mpptWatchdog.timeout)) {
        Log.errorln(F("[WATCHDOG_MPPTCHG] Fail to feed dog"));
        return false;
    }

    Log.infoln(F("[WATCHDOG_MPPTCHG] Dog fed"));

    lastFed = millis();

    return true;
}

bool WatchdogSlaveMpptChgThread::setManagedByUser(const uint64_t millis) {
    if (!_initiated && !begin()) {
        return false;
    }

    if (millis == 0) {
        managedByUser = true;
        return charger->setWatchdogEnable(false);
    }

    if (millis == 1) {
        managedByUser = false;
        return feed();
    }

    managedByUser = true;
    return charger->setWatchdogPoweroff(millis)
           && charger->setWatchdogTimeout(10)
           && charger->setWatchdogEnable(true);
}
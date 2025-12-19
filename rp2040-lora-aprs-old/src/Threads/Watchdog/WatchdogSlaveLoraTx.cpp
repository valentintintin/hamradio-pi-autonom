#include "Threads/Watchdog/WatchdogSlaveLoraTxThread.h"

#include "ArduinoLog.h"
#include "System.h"

WatchdogSlaveLoraTxThread::WatchdogSlaveLoraTxThread(System *system) : WatchdogThread(system, system->settings.lora.intervalTimeoutWatchdogTx, PSTR("WATCHDOG_LORA_TX")) {
    enabled = system->settings.lora.watchdogTxEnabled;
}

bool WatchdogSlaveLoraTxThread::runOnce() {
    if (isFed()) {
        Log.traceln(F("[WATCHDOG_LORA_TX] Dog fed"));
        return true;
    }

    if (!system->settings.lora.txEnabled) {
        return true;
    }

    Log.errorln(F("[WATCHDOG_LORA_TX] No TX for a long time, reboot"));
    system->planReboot();
    return true;
}

bool WatchdogSlaveLoraTxThread::feed() {
    Log.infoln(F("[WATCHDOG_LORA_TX] Feed dog"));
    lastFed = millis();
    return true;
}
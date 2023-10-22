#include "ArduinoLog.h"
#include "System.h"
#include "MpptMonitor.h"
#include <EEPROM.h>

MpptMonitor::MpptMonitor(System *system, TwoWire &wire) : system(system), wire(wire) {
}

bool MpptMonitor::begin() {
    if (!charger.begin(wire)) {
        system->serialError(PSTR("[MPPT] Charger error"));
        system->displayText(PSTR("Mppt error"), PSTR("Init failed"));

        return false;
    }

    init = true;
    useWatchdogSafety = true;

    return true;
}

bool MpptMonitor::update() {
    if (!timer.hasExpired()) {
        return false;
    }

    timer.restart();

    if (!init && !begin()) {
        return false;
    }

    bool hasImportantChange = false;
    bool oldBoolValue;

    Log.traceln(F("[MPPT] Fetch charger data"));

    if (!charger.getStatusValue(SYS_STATUS, &status)) {
        system->serialError(PSTR("[MPPT] Fetch charger data status error"));
        system->displayText(PSTR("Mttp error"), PSTR("Failed to fetch data status"));
        init = false;
        return false;
    } else {
        Log.verboseln(F("[MPPT] Charger status : %s"), mpptChg::getStatusAsString(status));
    }

    if (!charger.getIndexedValue(VAL_VS, &vs)) {
        system->serialError(PSTR("[MPPT] Fetch charger data VS error"));
        system->displayText(PSTR("Mttp error"), PSTR("Failed to fetch data VS"));
        init = false;
        return false;
    } else {
        Log.verboseln(F("[MPPT] Charger VS : %d"), vs);
    }

    if (!charger.getIndexedValue(VAL_IS, &is)) {
        system->serialError(PSTR("[MPPT] Fetch charger data IS error"));
        system->displayText(PSTR("Mttp error"), PSTR("Failed to fetch data IS"));
        init = false;
        return false;
    } else {
        Log.verboseln(F("[MPPT] Charger IS : %d"), is);
    }

    if (!charger.getIndexedValue(VAL_VB, &vb)) {
        system->serialError(PSTR("[MPPT] Fetch charger data VB error"));
        system->displayText(PSTR("Mttp error"), PSTR("Failed to fetch data VB"));
        init = false;
        return false;
    } else {
        Log.verboseln(F("[MPPT] Charger VB : %d"), vb);
    }

    if (!charger.getIndexedValue(VAL_IB, &ib)) {
        system->serialError(PSTR("[MPPT] Fetch charger data IB error"));
        system->displayText(PSTR("Mttp error"), PSTR("Failed to fetch data IB"));
        init = false;
        return false;
    } else {
        Log.verboseln(F("[MPPT] Charger IB : %d"), ib);
    }

    oldBoolValue = night;
    if (!charger.isNight(&night)) {
        system->serialError(PSTR("[MPPT] Fetch charger data night error"));
        system->displayText(PSTR("Mttp error"), PSTR("Failed to fetch data night"));
        init = false;
        return false;
    } else {
        Log.verboseln(F("[MPPT] Charger night : %d"), night);
        hasImportantChange |= oldBoolValue != night;
    }

    oldBoolValue = alert;
    if (!charger.isAlert(&alert)) {
        system->serialError(PSTR("[MPPT] Fetch charger data alert error"));
        system->displayText(PSTR("Mttp error"), PSTR("Failed to fetch data alert"));
        init = false;
        return false;
    } else {
        Log.verboseln(F("[MPPT] Charger alert : %d"), alert);
        hasImportantChange |= oldBoolValue != alert;
    }

    oldBoolValue = powerEnabled;
    if (!charger.isPowerEnabled(&powerEnabled)) {
        system->serialError(PSTR("[MPPT] Fetch charger data power enabled error"));
        system->displayText(PSTR("Mttp error"), PSTR("Failed to fetch data power enabled"));
        init = false;
        return false;
    } else {
        Log.verboseln(F("[MPPT] Charger power enabled : %d"), powerEnabled);
        hasImportantChange |= oldBoolValue != powerEnabled;
    }

    oldBoolValue = watchdogEnabled;
    if (!charger.getWatchdogEnable(&watchdogEnabled)) {
        system->serialError(PSTR("[MPPT] Fetch charger data watchdog error"));
        system->displayText(PSTR("Mttp error"), PSTR("Failed to fetch data watchdog"));
        init = false;
        return false;
    } else {
        Log.verboseln(F("[MPPT] Charger watchdog : %d"), watchdogEnabled);
        hasImportantChange |= oldBoolValue != watchdogEnabled;
    }

    uint16_t oldIntValue = watchdogPowerOffTime;
    if (!charger.getWatchdogPoweroff(&watchdogPowerOffTime)) {
        system->serialError(PSTR("[MPPT] Fetch charger data watchdog poweroff error"));
        system->displayText(PSTR("Mttp error"), PSTR("Failed to fetch data watchdog poweroff"));
        init = false;
        return false;
    } else {
        Log.verboseln(F("[MPPT] Charger watchdog poweroff : %d"), watchdogPowerOffTime);
        hasImportantChange |= oldIntValue != watchdogPowerOffTime;
    }

    if (!charger.getWatchdogTimeout(&watchdogCounter)) {
        system->serialError(PSTR("[MPPT] Fetch charger data watchdog counter error"));
        system->displayText(PSTR("Mttp error"), PSTR("Failed to fetch data watchdog counter"));
        init = false;
        return false;
    } else {
        Log.verboseln(F("[MPPT] Charger watchdog counter : %d"), watchdogCounter);
    }

    if (!charger.getConfigurationValue(CFG_PWR_OFF_TH, &powerOffVoltage)) {
        system->serialError(PSTR("[MPPT] Fetch charger data power off voltage error"));
        system->displayText(PSTR("Mttp error"), PSTR("Failed to fetch data power off voltage"));
        init = false;
        return false;
    } else {
        Log.verboseln(F("[MPPT] Charger power off voltage : %d"), powerOffVoltage);
    }

    if (!charger.getConfigurationValue(CFG_PWR_ON_TH, &powerOnVoltage)) {
        system->serialError(PSTR("[MPPT] Fetch charger data power on voltage error"));
        system->displayText(PSTR("Mttp error"), PSTR("Failed to fetch data power on voltage"));
        init = false;
        return false;
    } else {
        Log.verboseln(F("[MPPT] Charger power on voltage : %d"), powerOnVoltage);
    }

    serialJsonWriter
            .beginObject()
            .property(F("type"), PSTR("mppt"))
            .property(F("batteryVoltage"), vb)
            .property(F("batteryCurrent"), ib)
            .property(F("solarVoltage"), vs)
            .property(F("solarCurrent"), is)
            .property(F("currentCharge"), getCurrentCharge())
            .property(F("status"), status)
            .property(F("statusString"), (char*) mpptChg::getStatusAsString(status))
            .property(F("night"), night)
            .property(F("alert"), alert)
            .property(F("watchdogEnabled"), watchdogEnabled)
            .property(F("watchdogPowerOffTime"), watchdogPowerOffTime)
            .property(F("watchdogCounter"), watchdogCounter)
            .property(F("powerEnabled"), powerEnabled)
            .property(F("powerOffVoltage"), powerOffVoltage)
            .property(F("powerOnVoltage"), powerOnVoltage)
            .endObject(); SerialPiUsed.println();

    Log.infoln(F("[MPPT] Vb: %dmV Ib: %dmA Vs: %dmV Is: %dmA Ic: %dmA Status: %s Night: %d Alert: %d WD: %d WDOff: %ds WDCnt: %ds 5V: %d PowOffVolt: %d PowOnVolt: %d"), vb, ib, vs, is, getCurrentCharge(), mpptChg::getStatusAsString(status), night, alert, watchdogEnabled, watchdogPowerOffTime, watchdogCounter, powerEnabled, powerOffVoltage, powerOnVoltage);

    sprintf_P(bufferText, PSTR("Vb:%dmV Ib:%dmA Vs:%dmV Is:%dmA Ic:%dmA Status:%s PowOnVolt: %d"), vb, ib, vs, is, getCurrentCharge(), mpptChg::getStatusAsString(status), powerOnVoltage);
    system->displayText(PSTR("MPPT"), bufferText, 3000);

    sprintf_P(bufferText, PSTR("Night:%d Alert:%d WD:%d WDOff:%ds WDCnt:%ds 5V:%d PowOffVolt: %d"), night, alert, watchdogEnabled, watchdogPowerOffTime, watchdogCounter, powerEnabled, powerOffVoltage);
    system->displayText(PSTR("MPPT"), bufferText, 3000);

    timer.restart();

    if (system->isFunctionAllowed(EEPROM_ADDRESS_WATCHDOG_SAFETY)) {
        doWatchdogSafety();
    }

    if (hasImportantChange) {
        system->forceSendTelemetry = true;
    }

    return true;
}

bool MpptMonitor::setWatchdog(uint16_t powerOffTime, uint8_t timeoutTime) {
    if (!init) {
        if (!begin()) {
            return false;
        }
    }

    bool enabled = powerOffTime > 0;
    system->forceSendTelemetry = watchdogEnabled != enabled || powerOffTime != watchdogPowerOffTime;

    if (enabled) {
        if (!charger.setWatchdogPoweroff(powerOffTime)) {
            system->serialError(PSTR("[MPPT_WATCHDOG]Change watchdog poweroff error"));
            system->displayText(PSTR("Mttp error"), PSTR("Failed to set watchdog poweroff"));
            init = false;
            system->forceSendTelemetry = false;
            return false;
        }

        watchdogPowerOffTime = powerOffTime;

        if (!charger.setWatchdogTimeout(WATCHDOG_TIMEOUT)) {
            system->serialError(PSTR("[MPPT_WATCHDOG]Change watchdog counter error"));
            system->displayText(PSTR("Mttp error"), PSTR("Failed to set watchdog counter"));
            init = false;
            system->forceSendTelemetry = false;
            return false;
        }

        watchdogCounter = WATCHDOG_TIMEOUT;
    }

    if (!charger.setWatchdogEnable(enabled)) {
        system->serialError(PSTR("[MPPT_WATCHDOG]Change watchdog enable error"));
        system->displayText(PSTR("Mttp error"), PSTR("Failed to set watchdog"));
        init = false;
        system->forceSendTelemetry = false;
        return false;
    }

    if (!charger.getWatchdogEnable(&watchdogEnabled)) {
        system->serialError(PSTR("[MPPT] Fetch charger data watchdog error"));
        system->displayText(PSTR("Mttp error"), PSTR("Failed to fetch data watchdog"));
        init = false;
        system->forceSendTelemetry = false;
        return false;
    }

    useWatchdogSafety = !watchdogEnabled;
    timerWatchdogSafety.restart();

    sprintf_P(bufferText, PSTR("[MPPT_WATCHDOG] Watchdog state : %d. PowerOff : %d, Counter : %d"), enabled, powerOffTime, WATCHDOG_TIMEOUT);
    Log.infoln(bufferText);
    system->displayText(PSTR("WatchDog"), bufferText, 3000);

    timer.setExpired();

    return true;
}

bool MpptMonitor::setPowerOnOff(uint16_t powerOnVoltage, uint16_t powerOffVoltage) {
    sprintf_P(bufferText, PSTR("[MPPT_POWER] On : %dmV Off : %dmV"), powerOnVoltage, powerOffVoltage);
    Log.infoln(bufferText);
    system->displayText(PSTR("Power"), bufferText, 3000);

    return charger.setConfigurationValue(CFG_PWR_ON_TH, powerOnVoltage)
           && charger.setConfigurationValue(CFG_PWR_OFF_TH, powerOffVoltage);
}

void MpptMonitor::doWatchdogSafety() {
    if (alert) {
        useWatchdogSafety = false;
        return;
    }

    if (useWatchdogSafety && powerEnabled) {
        Log.traceln(F("[MPPT] Watchdog safety check, timer : %ld"), timerWatchdogSafety.getTimeLeft());

        if (timerWatchdogSafety.hasExpired()) {
            system->serialError(PSTR("[MPPT] Watchdog safety triggered ! Restart"));
            system->displayText(PSTR("Watchdog safety"), PSTR("Triggered so restart"));
            setWatchdog(1, 1);
        }
    }
}

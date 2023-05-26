#include "System.h"
#include "MpptMonitor.h"
#include "ArduinoLog.h"

MpptMonitor::MpptMonitor(System *system, TwoWire &wire) : system(system), wire(wire) {
}

bool MpptMonitor::begin() {
    if (!charger.begin(wire)) {
        Log.errorln(F("[MPPT] Charger error"));
        system->displayText(PSTR("Mppt error"), PSTR("Init failed"));

        return false;
    }

    init = true;

    return true;
}

bool MpptMonitor::update() {
    if (!timer.hasExpired()) {
        return false;
    }

    if (!init && !begin()) {
        timer.restart();
        return false;
    }

    Log.traceln(F("[MPPT] Fetch charger data"));

    if (!charger.getStatusValue(SYS_STATUS, &status)) {
        Log.errorln(F("[MPPT] Fetch charger data status error"));
        system->displayText(PSTR("Mttp error"), PSTR("Failed to fetch data status"));
        init = false;
        return false;
    } else {
        Log.traceln(F("[MPPT] Charger status : %s"), mpptChg::getStatusAsString(status));
    }

    if (!charger.getIndexedValue(VAL_VS, &vs)) {
        Log.errorln(F("[MPPT] Fetch charger data VS error"));
        system->displayText(PSTR("Mttp error"), PSTR("Failed to fetch data VS"));
        init = false;
        return false;
    } else {
        Log.traceln(F("[MPPT] Charger VS : %d"), vs);
    }

    if (!charger.getIndexedValue(VAL_IS, &is)) {
        Log.errorln(F("[MPPT] Fetch charger data IS error"));
        system->displayText(PSTR("Mttp error"), PSTR("Failed to fetch data IS"));
        init = false;
        return false;
    } else {
        Log.traceln(F("[MPPT] Charger IS : %d"), is);
    }

    if (!charger.getIndexedValue(VAL_VB, &vb)) {
        Log.errorln(F("[MPPT] Fetch charger data VB error"));
        system->displayText(PSTR("Mttp error"), PSTR("Failed to fetch data VB"));
        init = false;
        return false;
    } else {
        Log.traceln(F("[MPPT] Charger VB : %d"), vb);
    }

    if (!charger.getIndexedValue(VAL_IB, &ib)) {
        Log.errorln(F("[MPPT] Fetch charger data IB error"));
        system->displayText(PSTR("Mttp error"), PSTR("Failed to fetch data IB"));
        init = false;
        return false;
    } else {
        Log.traceln(F("[MPPT] Charger IB : %d"), ib);
    }

    if (!charger.isNight(&night)) {
        Log.errorln(F("[MPPT] Fetch charger data night error"));
        system->displayText(PSTR("Mttp error"), PSTR("Failed to fetch data night"));
        init = false;
        return false;
    } else {
        Log.traceln(F("[MPPT] Charger night : %d"), night);
    }

    if (!charger.isAlert(&alert)) {
        Log.errorln(F("[MPPT] Fetch charger data alert error"));
        system->displayText(PSTR("Mttp error"), PSTR("Failed to fetch data alert"));
        init = false;
        return false;
    } else {
        Log.traceln(F("[MPPT] Charger alert : %d"), alert);
    }

    if (!charger.isPowerEnabled(&powerEnabled)) {
        Log.errorln(F("[MPPT] Fetch charger data power enabled error"));
        system->displayText(PSTR("Mttp error"), PSTR("Failed to fetch data power enabled"));
        init = false;
        return false;
    } else {
        Log.traceln(F("[MPPT] Charger power enabled : %d"), powerEnabled);
    }

    if (!charger.getWatchdogEnable(&watchdogEnabled)) {
        Log.errorln(F("[MPPT] Fetch charger data watchdog error"));
        system->displayText(PSTR("Mttp error"), PSTR("Failed to fetch data watchdog"));
        init = false;
        return false;
    } else {
        Log.traceln(F("[MPPT] Charger watchdog : %d"), watchdogEnabled);
    }

    if (!charger.getWatchdogPoweroff(&watchdogPowerOffTime)) {
        Log.errorln(F("[MPPT] Fetch charger data watchdog poweroff error"));
        system->displayText(PSTR("Mttp error"), PSTR("Failed to fetch data watchdog poweroff"));
        init = false;
        return false;
    } else {
        Log.traceln(F("Charger watchdog poweroff : %d"), watchdogPowerOffTime);
    }

    if (!charger.getWatchdogTimeout(&watchdogCounter)) {
        Log.errorln(F("[MPPT] Fetch charger data watchdog counter error"));
        system->displayText(PSTR("Mttp error"), PSTR("Failed to fetch data watchdog counter"));
        init = false;
        return false;
    } else {
        Log.traceln(F("[MPPT] Charger watchdog counter : %d"), watchdogCounter);
    }

    if (!charger.getConfigurationValue(CFG_PWR_OFF_TH, &powerOffVoltage)) {
        Log.errorln(F("[MPPT] Fetch charger data power off voltage error"));
        system->displayText(PSTR("Mttp error"), PSTR("Failed to fetch data power off voltage"));
        init = false;
        return false;
    } else {
        Log.traceln(F("[MPPT] Charger watchdog counter : %d"), watchdogCounter);
    }

    if (!charger.getConfigurationValue(CFG_PWR_ON_TH, &powerOnVoltage)) {
        Log.errorln(F("[MPPT] Fetch charger data power on voltage error"));
        system->displayText(PSTR("Mttp error"), PSTR("Failed to fetch data power on voltage"));
        init = false;
        return false;
    } else {
        Log.traceln(F("[MPPT] Charger watchdog counter : %d"), watchdogCounter);
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
    system->displayText(PSTR("MPPT"), bufferText);

    sprintf_P(bufferText, PSTR("Night:%d Alert:%d WD:%d WDOff:%ds WDCnt:%ds 5V:%d PowOffVolt: %d"), night, alert, watchdogEnabled, watchdogPowerOffTime, watchdogCounter, powerEnabled, powerOffVoltage);
    system->displayText(PSTR("MPPT"), bufferText);

//    updateWatchdog();
//    checkAnormalCase();

    timer.restart();

    return true;
}

void MpptMonitor::updateWatchdog() {
    if (!init) return;

    if (timer.hasExpired() && watchdogEnabled) {
        Log.verboseln(F("[MPPT_WATCHDOG] counter set to %d"), WATCHDOG_TIMEOUT);

        if (!charger.setWatchdogTimeout(WATCHDOG_TIMEOUT)) {
            Log.errorln(F("[MPPT_WATCHDOG]Change watchdog counter error"));
            system->displayText(PSTR("Mttp error"), PSTR("Failed to set watchdog counter"));
            init = false;
        } else {
            timer.restart();
        }
    }
}

bool MpptMonitor::setWatchdog(uint32_t powerOffTime) {
    if (!init) {
        return false;
    }

    bool enabled = powerOffTime > 0;

    if (enabled) {
        if (!charger.setWatchdogPoweroff(powerOffTime)) {
            Log.errorln(F("[MPPT_WATCHDOG]Change watchdog poweroff error"));
            system->displayText(PSTR("Mttp error"), PSTR("Failed to set watchdog poweroff"));
            init = false;
            return false;
        }

        watchdogPowerOffTime = powerOffTime;

        if (!charger.setWatchdogTimeout(WATCHDOG_TIMEOUT)) {
            Log.errorln(F("[MPPT_WATCHDOG]Change watchdog counter error"));
            system->displayText(PSTR("Mttp error"), PSTR("Failed to set watchdog counter"));
            init = false;
            return false;
        }

        watchdogCounter = WATCHDOG_TIMEOUT;
    }

    if (!charger.setWatchdogEnable(enabled)) {
        Log.errorln(F("[MPPT_WATCHDOG]Change watchdog enable error"));
        system->displayText(PSTR("Mttp error"), PSTR("Failed to set watchdog"));
        init = false;
        return false;
    }

    watchdogEnabled = enabled;

    sprintf_P(bufferText, PSTR("[MPPT_WATCHDOG] Watchdog state : %d. PowerOff : %d, Counter : %d"), enabled, powerOffTime, WATCHDOG_TIMEOUT);
    Log.infoln(bufferText);
    system->displayText(PSTR("WatchDog"), bufferText, 3000);

    timer.setExpired();

    return true;
}

void MpptMonitor::checkAnormalCase() {
    if (!alert && !night && !watchdogEnabled && powerEnabled && vb > LOW_VOLTAGE) {
        Log.warningln(F("[MPPT_BRAIN] Strange state : all green but watchdog not running"));
        system->displayText(PSTR("WatchDog"), PSTR("Strange state : all green but watchdog not running"));
        setWatchdog(1);
    }
}

bool MpptMonitor::setPowerOnOff(uint16_t powerOnVoltage, uint16_t powerOffVoltage) {
    sprintf_P(bufferText, PSTR("[MPPT_POWER] On : %dmV Off : %dmV"), powerOnVoltage, powerOffVoltage);
    Log.infoln(bufferText);
    system->displayText(PSTR("Power"), bufferText, 3000);

    return charger.setConfigurationValue(CFG_PWR_ON_TH, powerOnVoltage)
           && charger.setConfigurationValue(CFG_PWR_OFF_TH, powerOffVoltage);
}

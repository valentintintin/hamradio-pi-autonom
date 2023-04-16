#include "MpptMonitor.h"
#include "ArduinoLog.h"

MpptMonitor::MpptMonitor(Communication *communication) : communication(communication) {
}

bool MpptMonitor::begin() {
    if (!chg.begin()) {
        Log.errorln(F("Charger error"));

        return false;
    }

    return true;
}

void MpptMonitor::update() {
    if (timerChg.hasExpired()) {
        Log.infoln(F("Fetch charger data"));

        if (!chg.getStatusValue(SYS_STATUS, &chgStatus)) {
            Log.errorln(F("Fetch charger data status error"));
        } else {
            Log.infoln(F("Charger status : %s"), mpptChg::getStatusAsString(chgStatus));
        }

        if (!chg.getIndexedValue(VAL_VS, &chgVs)) {
            Log.errorln(F("Fetch charger data VS error"));
        } else {
            Log.infoln(F("Charger VS : %d"), chgVs);
        }

        if (!chg.getIndexedValue(VAL_IS, &chgIs)) {
            Log.errorln(F("Fetch charger data IS error"));
        } else {
            Log.infoln(F("Charger IS : %d"), chgIs);
        }

        if (!chg.getIndexedValue(VAL_VB, &chgVb)) {
            Log.errorln(F("Fetch charger data VB error"));
        } else {
            Log.infoln(F("Charger VB : %d"), chgVb);
        }

        if (!chg.getIndexedValue(VAL_IB, &chgIb)) {
            Log.errorln(F("Fetch charger data IB error"));
        } else {
            Log.infoln(F("Charger IB : %d"), chgIb);
        }

        timerChg.restart();

        communication->sendTelemetry(PSTR(APRS_COMMENT), millis(), chgVs, chgIs, chgVb, chgIb, chgStatus);
    }
}

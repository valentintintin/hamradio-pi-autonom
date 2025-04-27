#include "Threads/Energy/EnergyMpptChgThread.h"
#include "ArduinoLog.h"
#include "System.h"

EnergyMpptChgThread::EnergyMpptChgThread(System *system, uint16_t *ocv, const size_t numOcvPoints) : EnergyThread(system, PSTR("ENERGY_MPPTCHG"), ocv, numOcvPoints) {
    charger = &system->mpptChgCharger;
}

bool EnergyMpptChgThread::init() {
    return charger->begin() && setPowerOnOff();
}

void EnergyMpptChgThread::run() {
    EnergyThread::run();
    fetchOthersData();

    if (system->settings.energy.sendAprsMessageWhenAlert && _isAlert && _isAlert != wasAlert) {
        system->communication.sendMessage(system->settings.energy.callsignToSendMessageAlert, PSTR("MPPT en alerte !"));
    }

    wasAlert = _isAlert;
}

bool EnergyMpptChgThread::fetchVoltageBattery() {
    return charger->getIndexedValue(VAL_VB, &vb);
}

bool EnergyMpptChgThread::fetchCurrentBattery() {
    return charger->getIndexedValue(VAL_IB, &ib);
}

bool EnergyMpptChgThread::fetchVoltageSolar() {
    return charger->getIndexedValue(VAL_VS, &vs);
}

bool EnergyMpptChgThread::fetchCurrentSolar() {
    return charger->getIndexedValue(VAL_IS, &is);
}

bool EnergyMpptChgThread::fetchOthersData() {
    return charger->isNight(&_isNight) && charger->isAlert(&_isAlert) && charger->getIndexedValue(VAL_INT_TEMP, &temperature);
}

bool EnergyMpptChgThread::setPowerOnOff() const {
    SettingsEnergy settings = system->settings.energy;
    
    Log.infoln(F("[ENERGY_MPPTCHG] Power On : %dmV and Power Off : %dmV"), settings.mpptPowerOnVoltage, settings.mpptPowerOffVoltage);

    if (!charger->setConfigurationValue(CFG_PWR_ON_TH, settings.mpptPowerOnVoltage)
        || !charger->setConfigurationValue(CFG_PWR_OFF_TH, settings.mpptPowerOffVoltage)) {
        Log.warningln(F("[ENERGY_MPPTCHG] Failed to set power on off"));
        return false;
    }

    uint16_t powerOffVoltage;
    uint16_t powerOnVoltage;

    if (charger->getConfigurationValue(CFG_PWR_OFF_TH, &powerOffVoltage)
        && charger->getConfigurationValue(CFG_PWR_ON_TH, &powerOnVoltage)
        && (settings.mpptPowerOnVoltage != powerOffVoltage || settings.mpptPowerOnVoltage != powerOnVoltage)
    ) {
        Log.warningln(F("[ENERGY_MPPTCHG] Power on off are different on charger ! WantOn: %dmV CurrentOn: %dmv, WantOff: %dmv CurrentOff: %dmv"),
            powerOnVoltage, settings.mpptPowerOnVoltage, powerOffVoltage, settings.mpptPowerOffVoltage);

        settings.mpptPowerOffVoltage = powerOffVoltage;
        settings.mpptPowerOnVoltage = powerOnVoltage;

        system->saveSettings();
    }

    return true;
}

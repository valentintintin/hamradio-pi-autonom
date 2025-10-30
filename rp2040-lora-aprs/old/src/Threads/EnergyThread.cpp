#include "ArduinoLog.h"
#include "System.h"
#include "Threads/EnergyThread.h"

#include <utils.h>

EnergyThread::EnergyThread(System *system, const char *name, uint16_t *ocv, const size_t numOcvPoints, const uint8_t numCells) : MyThread(system, system->settings.energy.intervalCheck, name),
numCells(numCells), ocv(ocv), numOcvPoints(numOcvPoints) {
    force = true;
}

uint8_t EnergyThread::getBatteryPercentage() const {
    if (numOcvPoints == 0 || ocv == nullptr) {
        return 0;
    }

    /**
         * @brief   Battery voltage lookup table interpolation to obtain a more
         * precise percentage rather than the old proportional one.
         * @author  Gabriele Russo
         * @date    06/02/2024
         */
    float battery_SOC = 0.0;
    const uint16_t voltage = getVoltageBattery() / numCells; // single cell voltage (average)

    for (uint8_t i = 0; i < numOcvPoints; i++) {
        if (ocv[i] <= voltage) {
            if (i == 0) {
                battery_SOC = 100.0; // 100% full
            } else {
                // interpolate between OCV[i] and OCV[i-1]
                battery_SOC = static_cast<float>(100.0) / (numOcvPoints - 1.0) *
                              (numOcvPoints - 1.0 - i + (static_cast<float>(voltage) - ocv[i]) / (ocv[i - 1] - ocv[i]));
            }
            break;
        }
    }

    return static_cast<uint8_t>(clamp(static_cast<uint16_t>(battery_SOC), 0, 100));
}

bool EnergyThread::runOnce() {
    Log.traceln(F("[ENERGY] Fetch charger data"));

    if (!fetchVoltageBattery()) {
        Log.errorln(F("[MPPT] Fetch charger data VB error"));
        return false;
    }

    if (!fetchCurrentBattery()) {
        Log.errorln(F("[MPPT] Fetch charger data IB error"));
        return false;
    }

    if (!fetchVoltageSolar()) {
        Log.errorln(F("[MPPT] Fetch charger data VS error"));
        return false;
    }

    if (!fetchCurrentSolar()) {
        Log.errorln(F("[MPPT] Fetch charger data IS error"));
        return false;
    }

    Log.infoln(F("[ENERGY] Vb: %dmV Ib: %dmA Vs: %dmV Is: %dmA Ic: %dmA Night: %T"), vb, ib, vs, is, getCurrentCharge(), isNight());

    return true;
}

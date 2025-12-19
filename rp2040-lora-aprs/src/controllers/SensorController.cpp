#include "../../include/controllers/SensorController.hpp"
#include "../../lib/mpptChg/mpptChg.h"

#ifdef ENV_INCLUDE_MPPTCHG
static mpptChg charger;
#endif

bool SensorController::begin() {
#ifdef ENV_INCLUDE_MPPTCHG
    mpptChg_initialized = charger.begin();
#endif

    return EnvironmentSensorManager::begin();;
}

bool SensorController::querySensors(uint8_t requester_permissions, CayenneLPP &telemetry) {
    const auto result = EnvironmentSensorManager::querySensors(requester_permissions, telemetry);

#ifdef ENV_INCLUDE_MPPTCHG
    if (mpptChg_initialized) {
        int16_t temperature;
        charger.getIndexedValue(VAL_INT_TEMP, &temperature);
        temperature /= 10;
        telemetry.addTemperature(next_available_channel, temperature);
        next_available_channel++;

        int16_t voltage, current;
        charger.getIndexedValue(VAL_VB, &voltage);
        charger.getIndexedValue(VAL_IB, &current);
        telemetry.addVoltage(next_available_channel, voltage);
        telemetry.addCurrent(next_available_channel, current);
        telemetry.addPower(next_available_channel, voltage * current);
        next_available_channel++;

        charger.getIndexedValue(VAL_VS, &voltage);
        charger.getIndexedValue(VAL_IS, &current);
        telemetry.addVoltage(next_available_channel, voltage);
        telemetry.addCurrent(next_available_channel, current);
        telemetry.addPower(next_available_channel, voltage * current);
        next_available_channel++;
    }
#endif

    return result;
}

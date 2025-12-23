/*
#include "meshcore/MeshCoreSensorManager.hpp"

bool MeshCoreSensorManager::querySensors(uint8_t requester_permissions, CayenneLPP &telemetry) {
    auto result = EnvironmentSensorManager::querySensors(requester_permissions, telemetry);

    // if (sensorController != nullptr)
    // {
    //     result &= sensorController->update();
    //
    //     const auto ourTelemetry = sensorController->getTelemetry();
    //
    //     telemetry.addTemperature(TELEM_CHANNEL_SELF, ourTelemetry.outdoorTemperature);
    //     telemetry.addRelativeHumidity(TELEM_CHANNEL_SELF, ourTelemetry.outdoorHumidity);
    //
    //     telemetry.addTemperature(next_available_channel, ourTelemetry.batteryTemperature);
    //     next_available_channel++;
    //
    //     telemetry.addVoltage(next_available_channel, ourTelemetry.batteryVoltage);
    //     telemetry.addCurrent(next_available_channel, ourTelemetry.batteryCurrent);
    //     telemetry.addPower(next_available_channel, ourTelemetry.batteryVoltage * ourTelemetry.batteryCurrent);
    //     next_available_channel++;
    //
    //     telemetry.addVoltage(next_available_channel, ourTelemetry.solarVoltage);
    //     telemetry.addCurrent(next_available_channel, ourTelemetry.solarCurrent);
    //     telemetry.addPower(next_available_channel, ourTelemetry.solarVoltage * ourTelemetry.solarCurrent);
    //     next_available_channel++;
    // }

    return result;
}
*/

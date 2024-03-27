#include "WeatherSensors.h"
#include "ArduinoLog.h"
#include "System.h"

WeatherSensors::WeatherSensors(System *system) : system(system) {
}

bool WeatherSensors::begin() {
    return sensor.begin();
}

bool WeatherSensors::update(bool force) {
    if (!force && !timer.hasExpired()) {
        return false;
    }

    timer.restart();

    Log.traceln(F("[WEATHER] Fetch weather sensors data"));

    temperature = sensor.read_temperature_c();
    humidity = sensor.read_humidity();
    pressure = sensor.read_pressure();

    serialJsonWriter
            .beginObject()
            .property(F("type"), PSTR("weather"))
            .property(F("temperature"), temperature)
            .property(F("humidity"), humidity)
            .property(F("pressure"), pressure)
            .endObject(); SerialPi->println();

    sprintf_P(bufferText, PSTR("Temperature: %.2fC Humidity: %.2f%% Pressure: %.2fhPa"), temperature, humidity, pressure);
    Log.infoln(PSTR("[WEATHER] %s"), bufferText);
    system->displayText(PSTR("Weather"), bufferText);

    timer.restart();

    return true;
}
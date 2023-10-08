#include "WeatherSensors.h"
#include "ArduinoLog.h"
#include "System.h"

WeatherSensors::WeatherSensors(System *system) : system(system) {
}

bool WeatherSensors::begin() {
    dht.setup(PIN_DHT, DHT::DHT22);

    return true;
}

bool WeatherSensors::update() {
    if (!timer.hasExpired()) {
        return false;
    }

    timer.restart();

    Log.traceln(F("Fetch weather sensors data"));

    delay(dht.getMinimumSamplingPeriod());

    dht.getTemperature();

    hasError = dht.getStatus() != DHT::ERROR_NONE;

    if (hasError) {
        system->serialError(PSTR("[WEATHER] Fetch DHT sensor error"));
        system->serialError(dht.getStatusString());
        system->displayText(PSTR("Weather error"), PSTR("Failed to fetch DHT sensor"));
        begin();
        return false;
    }

    temperature = dht.getTemperature();
    humidity = dht.getHumidity();

    serialJsonWriter
            .beginObject()
            .property(F("type"), PSTR("weather"))
            .property(F("temperature"), temperature)
            .property(F("humidity"), humidity)
            .endObject(); SerialPiUsed.println();

    sprintf_P(bufferText, PSTR("Temperature: %.2fC Humidity: %d%"), temperature, humidity);
    Log.infoln(PSTR("[WEATHER] %s"), bufferText);
    system->displayText(PSTR("Weather"), bufferText);

    timer.restart();

    return true;
}
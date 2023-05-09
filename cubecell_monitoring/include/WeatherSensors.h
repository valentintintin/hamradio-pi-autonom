#ifndef CUBECELL_MONITORING_WEATHERSENSORS_H
#define CUBECELL_MONITORING_WEATHERSENSORS_H

#include <DHT_U.h>

#include "Config.h"
#include "Timer.h"

class System;

class WeatherSensors {
public:
    explicit WeatherSensors(System *system);

    bool begin();
    bool update();

    inline double getTemperature() const {
        return temperature;
    }

    inline uint8_t getHumidity() const {
        return humidity;
    }
private:
    System *system;
    Timer timer = Timer(INTERVAL_WEATHER, true);

    DHT_Unified dht = DHT_Unified(PIN_DHT, DHT22);
    sensors_event_t event{};
    char bufferText[64]{};

    double temperature = 0;
    uint8_t humidity = 0;

    bool readDht();
};

#endif //CUBECELL_MONITORING_WEATHERSENSORS_H

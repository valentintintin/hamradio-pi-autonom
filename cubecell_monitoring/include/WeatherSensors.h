#ifndef CUBECELL_MONITORING_WEATHERSENSORS_H
#define CUBECELL_MONITORING_WEATHERSENSORS_H

#include "DHT.h"

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

    inline bool IsInError() const {
        return hasError;
    }
private:
    System *system;
    Timer timer = Timer(INTERVAL_WEATHER, true);

    DHT dht = DHT();
    char bufferText[40]{};

    double temperature = 0;
    uint8_t humidity = 0;
    bool hasError = true;
};

#endif //CUBECELL_MONITORING_WEATHERSENSORS_H

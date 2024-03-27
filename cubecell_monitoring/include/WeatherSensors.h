#ifndef CUBECELL_MONITORING_WEATHERSENSORS_H
#define CUBECELL_MONITORING_WEATHERSENSORS_H

#include "vt_bme280"

#include "Config.h"
#include "Timer.h"

class System;
using namespace vt;

class WeatherSensors {
public:
    explicit WeatherSensors(System *system);

    bool begin();
    bool update(bool force = false);

    inline float getTemperature() const {
        return temperature;
    }

    inline float getHumidity() const {
        return humidity;
    }

    inline float getPressure() const {
        return pressure;
    }
private:
    System *system;
    Timer timer = Timer(INTERVAL_WEATHER, true);

    bme280_t sensor = bme280_t();

    float temperature = 0;
    float humidity = 0;
    float pressure = 0;
};

#endif //CUBECELL_MONITORING_WEATHERSENSORS_H

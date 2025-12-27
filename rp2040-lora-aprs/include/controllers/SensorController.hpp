#pragma once

#include "BaseController.hpp"
#include "telemetry.h"
#include "hal/MpptChargerHal.hpp"

#include <Adafruit_BME280.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_INA3221.h>
#include <timers.h>
#include <JsonWriter.h>

#include "hal/I2CSlaveHal.hpp"

#define QUERY_DELAY 30000

struct StreamJson
{
    JsonWriter jsonWriter;
    Stream *stream;
};

class SensorController : BaseController
{
public:
    static SensorController& getInstance()
    {
        static SensorController instance;
        return instance;
    }

    bool begin() override;
    bool queryTelemetries();
    void printJson(StreamJson& streamJson);

    bool isInitialized() const
    {
        return mpptChgInitialized || bmp280Initialized || bme280Initialized || ina3221Initialized || i2cSlaveInitialized;
    }

    static const Telemetry& getTelemetry()
    {
        return telemetry;
    }

    static void queryTimer(TimerHandle_t timer);

private:
    static Telemetry telemetry;

    TimerHandle_t timer;

    bool mpptChgInitialized = false;
    MpptChargerHal charger = MpptChargerHal::getInstance();

    bool bmp280Initialized = false;
    Adafruit_BMP280 bmp280 = Adafruit_BMP280();

    bool bme280Initialized = false;
    Adafruit_BME280 bme280 = Adafruit_BME280();

    bool ina3221Initialized = false;
    Adafruit_INA3221 ina3221 = Adafruit_INA3221();

    bool i2cSlaveInitialized = false;
    I2CSlaveHal i2cSlave = I2CSlaveHal();

    StreamJson serialJson = {
        .jsonWriter = JsonWriter(&Serial),
        .stream = &Serial
    };
    StreamJson serial1Json = {
        .jsonWriter = JsonWriter(&Serial1),
        .stream = &Serial1
    };
    StreamJson serial2Json = {
        .jsonWriter = JsonWriter(&Serial2),
        .stream = &Serial2
    };

    SensorController();

    bool initMpptCharger();
    bool initIna3221();
    bool initBme280();
    bool initBmp280();
    bool initI2cSlave();
};

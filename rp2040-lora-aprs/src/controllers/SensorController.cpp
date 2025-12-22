#include "controllers/SensorController.hpp"

#include <ArduinoLog.h>

#include "utils/utils.h"

Telemetry SensorController::telemetry{};

SensorController::SensorController()
{
    timer = xTimerCreate("queryTelemetries", pdMS_TO_TICKS(QUERY_DELAY), pdTRUE, this, queryTimer);

    if (timer == nullptr)
    {
        Log.warningln("Timer creation failed");
    }
}

void SensorController::queryTimer(TimerHandle_t timer)
{
    const auto ctrl = static_cast<SensorController*>(pvTimerGetTimerID(timer));
    ctrl->queryTelemetries();
}

bool SensorController::begin()
{
    return initMpptCharger() || initIna3221() || initBme280() || initBmp280();
}

bool SensorController::queryTelemetries()
{
    bool result = false;

    telemetry.updatedAt = getDateTime().unixtime();

    if (mpptChgInitialized)
    {
        Log.infoln("Query MpptCharger");

        if (!charger.queryTelemetries(telemetry.battery, telemetry.solar, telemetry.batteryTemperature))
        {
            Log.warningln("MpptCharger in error");

            initMpptCharger();
        }
        else
        {
            result = true;
        }
    }

    if (ina3221Initialized)
    {
        Log.infoln("Query INA3221");

        telemetry.battery.voltage = ina3221.getBusVoltage(0);
        telemetry.battery.current = ina3221.getCurrentAmps(0) * 1000;

        telemetry.solar.voltage = ina3221.getBusVoltage(1);
        telemetry.solar.current = ina3221.getCurrentAmps(1) * 1000;

        if (telemetry.solar.current == NAN)
        {
            Log.warningln("INA3221 in error");

            initIna3221();
        }
        else
        {
            result = true;
        }
    }

    if (bmp280Initialized)
    {
        Log.infoln("Query BMP280");

        telemetry.box.temperature = bmp280.readTemperature();
        telemetry.box.pressure = bmp280.readPressure();

        if (telemetry.box.pressure == NAN)
        {
            Log.warningln("BMP280 in error");

            initBmp280();
        }
        else
        {
            result = true;
        }
    }

    if (bme280Initialized)
    {
        Log.infoln("Query BME280");

        telemetry.box.temperature = bme280.readTemperature();
        telemetry.box.humidity = bme280.readHumidity();
        telemetry.box.pressure = bme280.readPressure();

        if (telemetry.box.pressure == NAN)
        {
            Log.warningln("BME280 in error");

            initBme280();
        }
        else
        {
            result = true;
        }
    }

    return result;
}

bool SensorController::initMpptCharger()
{
    Log.infoln("MpptCharger try init");

    mpptChgInitialized = charger.begin();
    if (mpptChgInitialized)
    {
        Log.info("MpptCharger found");
    }
    else
    {
        Log.infoln("MpptCharger init failed");
    }

    return mpptChgInitialized;
}

bool SensorController::initIna3221()
{
    Log.infoln("INA3221 try init");

    ina3221Initialized = ina3221.begin();

    if (ina3221Initialized)
    {
        Log.info("INA3221 found");
    }
    else
    {
        Log.infoln("INA3221 init failed");
    }

    return ina3221Initialized;
}

bool SensorController::initBme280()
{
    Log.infoln("BME280 try init");

    bme280Initialized = bme280.begin();

    if (bme280Initialized)
    {
        bme280.setSampling(Adafruit_BME280::MODE_FORCED,
                           Adafruit_BME280::SAMPLING_X1, // Temp. oversampling
                           Adafruit_BME280::SAMPLING_X1, // Pressure oversampling
                           Adafruit_BME280::SAMPLING_X1, // Humidity oversampling
                           Adafruit_BME280::FILTER_OFF, Adafruit_BME280::STANDBY_MS_1000);

        Log.info("BME280 found");
    }
    else
    {
        Log.infoln("BME280 init failed");
    }

    return bme280Initialized;
}


bool SensorController::initBmp280()
{
    Log.infoln("BMP280 try init");

    bmp280Initialized = bmp280.begin();
    if (bmp280.begin())
    {
        bmp280.setSampling(Adafruit_BMP280::MODE_FORCED,
                           Adafruit_BMP280::SAMPLING_X1, // Temp. oversampling
                           Adafruit_BMP280::SAMPLING_X1, // Pressure oversampling
                           Adafruit_BMP280::FILTER_OFF, Adafruit_BMP280::STANDBY_MS_1000);

        Log.info("BMP280 found");
    }
    else
    {
        Log.infoln("BMP280 init failed");
    }

    return bmp280Initialized;
}

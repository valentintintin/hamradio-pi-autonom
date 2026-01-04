#include "controllers/SensorController.hpp"

#include <ArduinoLog.h>

#include "SettingsManager.hpp"
#include "controllers/LedController.hpp"
#include "hal/I2CMasterHal.hpp"
#include "utils/utils.h"

Telemetry SensorController::telemetry{};

SensorController::SensorController()
{
    timer = xTimerCreate("queryTelemetries", pdMS_TO_TICKS(QUERY_DELAY), pdTRUE, this, queryTimer);

    if (timer == nullptr)
    {
        Log.warningln("Timer creation failed");

        LedController::getInstance().blink(Error, FreeRtos);
    }
}

void SensorController::queryTimer(const TimerHandle_t timer)
{
    const auto ctrl = static_cast<SensorController*>(pvTimerGetTimerID(timer));
    ctrl->queryTelemetries();
}

bool SensorController::begin()
{
    Wire.begin();
    xTimerStart(timer, 0);

    if (!I2CMasterHal::takeSemaphore())
    {
        Log.warningln("Sensor can not begin, can not have semaphore");

        LedController::getInstance().blink(Error, I2C);

        return false;
    }

    const auto result = initMpptCharger() || initIna3221() || initBme280() || initBmp280() || initI2cSlave();

    I2CMasterHal::releaseSemaphore();

    if (result)
    {
        LedController::getInstance().blink(Success, Sensor);
    }

    return result;
}

bool SensorController::queryTelemetries()
{
    if (!isInitialized() && !begin())
    {
        Log.warningln("Sensor can not query, no one found");

        return false;
    }

    if (!I2CMasterHal::takeSemaphore())
    {
        Log.warningln("Sensor can not query, can not have semaphore");

        LedController::getInstance().blink(Error, I2C);

        return false;
    }

    bool result = false;

    telemetry.updatedAt = getDateTime().unixtime();

    if (mpptChgInitialized)
    {
        Log.infoln("Query MpptCharger");

        if (!charger.queryTelemetries(telemetry.battery, telemetry.solar, telemetry.batteryTemperature))
        {
            Log.warningln("MpptCharger in error");

            LedController::getInstance().blink(Error, Sensor);

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

            LedController::getInstance().blink(Error, Sensor);

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
        telemetry.box.pressure = bmp280.readPressure() / 100.0;

        if (telemetry.box.pressure == NAN)
        {
            Log.warningln("BMP280 in error");

            LedController::getInstance().blink(Error, Sensor);

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
        telemetry.box.pressure = bme280.readPressure() / 100.0;

        if (telemetry.box.pressure == NAN)
        {
            Log.warningln("BME280 in error");

            LedController::getInstance().blink(Error, Sensor);

            initBme280();
        }
        else
        {
            result = true;
        }
    }

    if (i2cSlaveInitialized)
    {
        Log.infoln("Query I2CSlave");

        if (i2cSlave.queryTelemetries(telemetry))
        {
            result = true;
        }
        else
        {
            Log.warningln("I2CSlave in error");

            LedController::getInstance().blink(Error, Sensor);

            initI2cSlave();
        }
    }

    I2CMasterHal::releaseSemaphore();

    if (result)
    {
        LedController::getInstance().blink(Success, Sensor);

        printJson(serialJson);
        printJson(serial1Json);
        printJson(serial2Json);
    }

    return result;
}

void SensorController::printJson(StreamJson& streamJson)
{
    streamJson.jsonWriter.beginObject()
    .property("updatedAt", telemetry.updatedAt)

    .beginObject("battery")
        .property("voltage", telemetry.battery.voltage)
        .property("current", telemetry.battery.current)
    .endObject()

    .beginObject("solar")
        .property("voltage", telemetry.solar.voltage)
        .property("current", telemetry.solar.current)
    .endObject()

    .property("batteryTemperature", telemetry.batteryTemperature)

    .beginObject("box")
        .property("temperature", telemetry.box.temperature)
        .property("humidity", telemetry.box.humidity)
        .property("pressure", telemetry.box.pressure)
    .endObject()

    .beginObject("outdoor")
        .beginObject("basic")
            .property("temperature", telemetry.outdoor.basic.temperature)
            .property("humidity", telemetry.outdoor.basic.humidity)
            .property("pressure", telemetry.outdoor.basic.pressure)
        .endObject()

        .property("rain", telemetry.outdoor.rain)

        .beginObject("wind")
            .property("direction", telemetry.outdoor.wind.direction)
            .property("speedAverage", telemetry.outdoor.wind.speedAverage)
            .property("speedMax", telemetry.outdoor.wind.speedMax)
        .endObject()

        .beginObject("light")
            .property("uv", telemetry.outdoor.light.uv)
            .property("uvIndex", telemetry.outdoor.light.uvIndex)
            .property("lux", telemetry.outdoor.light.lux)
        .endObject()
    .endObject()

    .beginObject("position")
        .property("latitude", telemetry.position.latitude)
        .property("longitude", telemetry.position.longitude)
        .property("altitude", telemetry.position.altitude)
    .endObject()

.endObject();

    streamJson.stream.println();
}

bool SensorController::initMpptCharger()
{
    Log.infoln("MpptCharger try init");

    mpptChgInitialized = charger.begin();
    if (mpptChgInitialized)
    {
        Log.infoln("MpptCharger found");
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
        Log.infoln("INA3221 found");
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

        Log.infoln("BME280 found");
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

        Log.infoln("BMP280 found");
    }
    else
    {
        Log.infoln("BMP280 init failed");
    }

    return bmp280Initialized;
}

bool SensorController::initI2cSlave()
{
    Log.infoln("I2C Slave try init");

    i2cSlaveInitialized = i2cSlave.begin(SettingsManager::getSettings().i2c.address);
    if (i2cSlaveInitialized)
    {
        Log.infoln("I2C Slave found");
    }
    else
    {
        Log.infoln("I2C Slave init failed");
    }

    return i2cSlaveInitialized;
}

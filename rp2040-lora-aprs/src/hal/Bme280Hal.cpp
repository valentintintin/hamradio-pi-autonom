#include "hal/Bme280Hal.hpp"

Bme280Hal::Bme280Hal(const uint8_t address) : address(address)
{
}

bool Bme280Hal::begin()
{
    if (initialized)
    {
        return true;
    }

    Log.infoln("BME280 address %d init", address);

    initialized = bme280.begin(address);

    if (initialized)
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

    return initialized;
}

bool Bme280Hal::query(Telemetry& telemetry)
{
    Log.infoln("Query BME280");

    telemetry.box.temperature = bme280.readTemperature();
    telemetry.box.humidity = bme280.readHumidity();
    telemetry.box.pressure = bme280.readPressure() / 100.0;

    if (telemetry.box.pressure == NAN)
    {
        Log.warningln("BME280 in error");

        LedController::getInstance().blink(LedError, LedSensor);

        initialized = false;

        return false;
    }

    return true;
}
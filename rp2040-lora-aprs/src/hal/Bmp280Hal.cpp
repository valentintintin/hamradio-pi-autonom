#include "hal/Bmp280Hal.hpp"

Bmp280Hal::Bmp280Hal(const uint8_t address) : address(address)
{
}

bool Bmp280Hal::begin()
{
    if (initialized)
    {
        return true;
    }

    Log.infoln("BMP280 address %d init", address);

    initialized = bmp280.begin(address);

    if (initialized)
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

    return initialized;
}

bool Bmp280Hal::query(Telemetry& telemetry)
{
    Log.infoln("Query BMP280");

    telemetry.box.temperature = bmp280.readTemperature();
    telemetry.box.pressure = bmp280.readPressure() / 100.0;

    if (telemetry.box.pressure == NAN)
    {
        Log.warningln("BMP280 in error");

        LedController::getInstance().blink(Error, Sensor);

        initialized = false;

        return false;
    }

    return true;
}

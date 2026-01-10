#pragma once

#include "hal/SensorHal.hpp"

#include "Adafruit_BMP280.h"

class Bmp280Hal : public SensorHal
{
public:
    static Bmp280Hal& getInstance(const uint8_t address = BMP280_ADDRESS)
    {
        switch (address)
        {
        default:
        case BMP280_ADDRESS:
            static Bmp280Hal instancePrimaryAddress(address);
            return instancePrimaryAddress;
        case BMP280_ADDRESS_ALT:
            static Bmp280Hal instanceAlternateAddress(address);
            return instanceAlternateAddress;
        }
    }

    bool begin() override;
    bool query(Telemetry &telemetry) override;
private:
    Adafruit_BMP280 bmp280 = Adafruit_BMP280();
    uint8_t address;

    explicit Bmp280Hal(uint8_t address);
};

#pragma once

#include "hal/SensorHal.hpp"

#include "Adafruit_BME280.h"

class Bme280Hal : public SensorHal
{
public:
    static Bme280Hal& getInstance(const uint8_t address = BME280_ADDRESS)
    {
        switch (address)
        {
        default:
        case BME280_ADDRESS:
            static Bme280Hal instancePrimaryAddress(address);
            return instancePrimaryAddress;
        case BME280_ADDRESS_ALTERNATE:
            static Bme280Hal instanceAlternateAddress(address);
            return instanceAlternateAddress;
        }
    }

    bool begin() override;
    bool query(Telemetry &telemetry) override;
private:
    Adafruit_BME280 bme280 = Adafruit_BME280();
    uint8_t address;

    explicit Bme280Hal(uint8_t address);
};

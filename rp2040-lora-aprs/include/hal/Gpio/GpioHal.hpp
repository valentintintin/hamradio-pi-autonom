#pragma once

#include <Arduino.h>

#include "hal/Hal.hpp"

class GpioHal : public Hal
{
public:
    GpioHal(uint8_t pin, PinMode mode, bool inverted = false, bool useI2C = false, GpioHal* gpioToLow = nullptr);

    bool begin() override;
    PinStatus set(PinStatus state);
    PinStatus toggle();
    PinStatus read();

    PinStatus getPinStatus() const
    {
        return pinStatus;
    }

    const uint8_t pin;
    const PinMode mode;
    const bool inverted;
    const bool useI2C;
    GpioHal* gpioToLow = nullptr;

protected:
    virtual bool doSet(PinStatus state) = 0;
    virtual PinStatus doRead() = 0;

private:
    PinStatus pinStatus = LOW;
};

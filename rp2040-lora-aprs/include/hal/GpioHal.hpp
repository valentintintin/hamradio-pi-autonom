#pragma once

#include <Arduino.h>

class GpioHal
{
public:
    GpioHal(uint8_t pin, PinMode mode, bool inverted = false, bool useI2C = false, GpioHal* gpioToLow = nullptr);

    bool init();
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

    virtual ~GpioHal() = default;

protected:
    virtual bool doInit() = 0;
    virtual bool doSet(PinStatus state) = 0;
    virtual PinStatus doRead() = 0;

private:
    PinStatus pinStatus = LOW;
};

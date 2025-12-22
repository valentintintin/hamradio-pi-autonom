#pragma once

#include <Arduino.h>

class GpioHal
{
public:
    GpioHal(uint8_t pin, PinMode mode, bool inverted = false);
    virtual bool init() = 0;
    virtual bool set(bool state) = 0;
    virtual bool get() = 0;

    bool toggle();

    const uint8_t pin;
    const PinMode mode;
    const bool inverted;
};

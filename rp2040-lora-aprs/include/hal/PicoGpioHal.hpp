#pragma once
#include "GpioHal.hpp"

class PicoGpioHal : public GpioHal
{
public:
    explicit PicoGpioHal(uint8_t pin, PinMode mode, bool inverted = false, GpioHal* gpioToLow = nullptr);
protected:
    bool doInit() override;
    bool doSet(PinStatus state) override;
    PinStatus doRead() override;
};

#pragma once
#include "Tca9555Hal.hpp"
#include "hal/GpioHal.hpp"

class Tca9555GpioHal : public GpioHal
{
public:
    explicit Tca9555GpioHal(Tca9555Hal &tca, uint8_t pin, PinMode mode, bool inverted = false, GpioHal* gpioToLow = nullptr);
protected:
    bool doInit() override;
    bool doSet(PinStatus state) override;
    PinStatus doRead() override;
private:
    Tca9555Hal &tca;
};

#pragma once
#include "Tca9555Hal.hpp"
#include "hal/GpioHal.hpp"

class Tca9555GpioHal : public GpioHal
{
public:
    explicit Tca9555GpioHal(Tca9555Hal &tca, uint8_t pin, PinMode mode, bool inverted = false);
    bool init() override;
    bool set(bool state) override;
    bool get() override;

private:
    Tca9555Hal &tca;
};

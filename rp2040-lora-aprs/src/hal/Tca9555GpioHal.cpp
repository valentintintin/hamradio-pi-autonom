#include "hal/Tca9555GpioHal.hpp"

Tca9555GpioHal::Tca9555GpioHal(Tca9555Hal& tca, const uint8_t pin, const PinMode mode, const bool inverted, GpioHal* gpioToLow) :
GpioHal(pin, mode, inverted, true, gpioToLow), tca(tca)
{
}

bool Tca9555GpioHal::doInit()
{
    return tca.setMode(pin, mode);
}

bool Tca9555GpioHal::doSet(const PinStatus state)
{
    return tca.write(pin, state);
}

PinStatus Tca9555GpioHal::doRead()
{
    return tca.read(pin);
}

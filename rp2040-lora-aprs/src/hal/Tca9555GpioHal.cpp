#include "hal/Tca9555GpioHal.hpp"

Tca9555GpioHal::Tca9555GpioHal(Tca9555Hal& tca, const uint8_t pin, const PinMode mode, const bool inverted) : GpioHal(pin, mode, inverted, true), tca(tca)
{
}

bool Tca9555GpioHal::init()
{
    return tca.setMode(pin, mode);
}

bool Tca9555GpioHal::set(const bool state)
{
    return tca.write(pin, state);
}

bool Tca9555GpioHal::get()
{
    return tca.read(pin) == HIGH;
}

#include "hal/Tca9555GpioHal.hpp"

Tca9555GpioHal::Tca9555GpioHal(Tca9555Hal& tca, uint8_t pin, PinMode mode, bool inverted) : tca(tca), GpioHal(pin, mode, inverted)
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

#include "hal/Gpio/PicoGpioHal.hpp"

#include <ArduinoLog.h>

PicoGpioHal::PicoGpioHal(const uint8_t pin, const PinMode mode, const bool inverted, GpioHal* gpioToLow) : GpioHal(pin, mode, inverted, false, gpioToLow)
{
}

bool PicoGpioHal::doBegin()
{
    pinMode(pin, mode);
    return true;
}

bool PicoGpioHal::doSet(const PinStatus level)
{
    digitalWrite(pin, level);
    return true;
}

PinStatus PicoGpioHal::doRead()
{
    return digitalRead(pin);
}
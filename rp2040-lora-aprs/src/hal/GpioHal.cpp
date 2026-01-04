#include "hal/GpioHal.hpp"

#include <ArduinoLog.h>

GpioHal::GpioHal(const uint8_t pin, const PinMode mode, const bool inverted, const bool useI2C) : pin(pin), mode(mode), inverted(inverted), useI2C(useI2C)
{
}

bool GpioHal::toggle()
{
    Log.infoln("Pin %d toggled", pin);
    return set(!get());
}

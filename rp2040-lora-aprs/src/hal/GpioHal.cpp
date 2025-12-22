#include "hal/GpioHal.hpp"

#include <ArduinoLog.h>

GpioHal::GpioHal(const uint8_t pin, const PinMode mode, const bool inverted) : pin(pin), mode(mode), inverted(inverted)
{
}

bool GpioHal::toggle()
{
    Log.infoln(F("Pin %d toggled"), pin);
    return set(!get());
}

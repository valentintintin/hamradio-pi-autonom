#include "../include/PicoGpioHal.hpp"

#include <ArduinoLog.h>

PicoGpioHal::PicoGpioHal(const uint8_t pin, const PinMode mode, const bool inverted) : GpioHal(pin, mode, inverted) {
}

bool PicoGpioHal::init() {
    Log.infoln(F("Pin %d set mode to %d"), pin, mode);
    pinMode(pin, mode);
    return true;
}

bool PicoGpioHal::set(const bool level) {
    Log.infoln(F("Pin %d set to %T"), pin, level);
    digitalWrite(pin, inverted ? !level : level);
    return true;
}

bool PicoGpioHal::get() {
    const auto level = digitalRead(pin) == (inverted ? LOW : HIGH);
    Log.infoln(F("Pin %d get value %T"), pin, level);
    return level;
}

#include "../include/PicoGpioHal.hpp"

PicoGpioHal::PicoGpioHal(const uint8_t pin, const PinMode mode, const bool inverted) : GpioHal(pin, mode, inverted) {
}

bool PicoGpioHal::init() {
    pinMode(pin, mode);
    return true;
}

bool PicoGpioHal::set(const bool level) {
    digitalWrite(pin, inverted ? !level : level);
    return true;
}

bool PicoGpioHal::get() {
    return digitalRead(pin) == (inverted ? LOW : HIGH);
}

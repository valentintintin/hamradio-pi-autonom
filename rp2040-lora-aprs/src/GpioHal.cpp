#include "GpioHal.hpp"

GpioHal::GpioHal(const uint8_t pin, const PinMode mode, const bool inverted) : pin(pin), mode(mode), inverted(inverted) {
}

bool GpioHal::toggle() {
    return set(!get());
}

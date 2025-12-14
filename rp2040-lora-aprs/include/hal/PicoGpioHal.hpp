#pragma once
#include "GpioHal.hpp"

class PicoGpioHal : public GpioHal {
public:
    explicit PicoGpioHal(uint8_t pin, PinMode mode, bool inverted = false);
    bool init() override;
    bool set(bool level) override;
    bool get() override;
};

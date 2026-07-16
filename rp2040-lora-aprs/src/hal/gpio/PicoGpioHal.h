#pragma once

#include "hal/gpio/GpioHal.h"

// GpioHal — broche digitale native du RP2040 (pinMode/digitalWrite/digitalRead).
class PicoGpioHal : public GpioHal {
public:
  PicoGpioHal(uint8_t pin, uint8_t mode) : _pin(pin), _mode(mode) {}

  bool begin() override;
  bool set(bool level) override;
  bool read(bool& level) override;

private:
  uint8_t _pin;
  uint8_t _mode;
};

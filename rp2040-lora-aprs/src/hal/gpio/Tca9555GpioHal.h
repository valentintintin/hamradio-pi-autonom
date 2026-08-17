#pragma once

#include "hal/gpio/GpioHal.h"
#include "Tca9555Hal.h"

// GpioHal — broche (0-15) d'un expandeur I2C TCA9555.
class Tca9555GpioHal : public GpioHal {
public:
  Tca9555GpioHal(Tca9555Hal& expander, uint8_t pin, bool asOutput)
    : _expander(&expander), _pin(pin), _asOutput(asOutput) {}

  bool begin() override;
  bool set(bool level) override;
  bool read(bool& level) override;
  bool pulse(bool level, uint16_t ms) override;

private:
  Tca9555Hal* _expander;
  uint8_t _pin;
  bool _asOutput;
};

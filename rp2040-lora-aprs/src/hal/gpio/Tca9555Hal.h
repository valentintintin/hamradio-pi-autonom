#pragma once

#include "hal/i2c/I2CBus.h"
#include <TCA9555.h>
#include <stdint.h>

class Tca9555Hal {
public:
  Tca9555Hal(I2CBus& bus, uint8_t addr)
    : _bus(&bus), _tca(addr, &bus.wire()) {}

  bool isConnected();
  bool writeOutputPort(uint8_t port, uint8_t value);
  bool writeConfigPort(uint8_t port, uint8_t configMask);
  bool setPinMode(uint8_t pin, bool asOutput);
  bool writePin(uint8_t pin, bool level);
  bool readPin(uint8_t pin, bool& level);
  bool pulsePin(uint8_t pin, bool level, uint16_t ms);

private:
  I2CBus* _bus;
  TCA9555 _tca;
};

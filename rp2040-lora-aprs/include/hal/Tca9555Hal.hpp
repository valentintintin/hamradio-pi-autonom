#pragma once

#include <TCA9555.h>

class Tca9555Hal
{
public:
    explicit Tca9555Hal(uint8_t address);
    bool begin();

    bool setMode(uint8_t pin, PinMode mode);
    bool write(uint8_t pin, bool value);
    PinStatus read(uint8_t pin);
private:
    bool initialized = false;
    TCA9555 tca;
};
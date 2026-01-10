#pragma once

#include <TCA9555.h>

class Tca9555Hal
{
public:
    static Tca9555Hal& getInstance(const uint8_t address)
    {
        switch (address)
        {
        case 0x21:
            static Tca9555Hal instance21(address);
            return instance21;
        case 0x22:
            static Tca9555Hal instance22(address);
            return instance22;
        case 0x23:
            static Tca9555Hal instance23(address);
            return instance23;
        case 0x24:
            static Tca9555Hal instance24(address);
            return instance24;
        case 0x25:
            static Tca9555Hal instance25(address);
            return instance25;
        case 0x26:
            static Tca9555Hal instance26(address);
            return instance26;
        case 0x27:
            static Tca9555Hal instance27(address);
            return instance27;
        default:
        case 0x20:
            static Tca9555Hal instance20(address);
            return instance20;
        }
    }

    bool begin();

    bool setMode(uint8_t pin, PinMode mode);
    bool write(uint8_t pin, bool value);
    PinStatus read(uint8_t pin);

    uint8_t getAddress()
    {
        return tca.getAddress();
    }
private:
    explicit Tca9555Hal(uint8_t address);

    bool initialized = false;
    TCA9555 tca;
};
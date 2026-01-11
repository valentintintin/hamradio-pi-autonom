#pragma once

#include <JC_EEPROM.h>

class M24M02Hal
{
public:
    static M24M02Hal& getInstance()
    {
        static M24M02Hal instance;
        return instance;
    }

    bool begin();
    bool read(uint32_t addr, uint8_t* values, size_t size);
    uint8_t read(uint32_t addr);
    bool write(uint32_t addr, uint8_t* values, size_t size);
    bool write(uint32_t addr, uint8_t value);
    uint8_t update(uint32_t addr, uint8_t value);

private:
    JC_EEPROM eeprom = JC_EEPROM(JC_EEPROM::kbits_256, 1, 256);
    bool initialized = false;
};
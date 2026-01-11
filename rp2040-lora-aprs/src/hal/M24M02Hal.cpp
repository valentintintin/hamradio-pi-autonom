#include "hal/M24M02Hal.hpp"

#include <ArduinoLog.h>
#include "controllers/LedController.hpp"

bool M24M02Hal::begin()
{
    if (initialized)
    {
        return true;
    }

    Log.infoln("M24M02 init");

    const auto result = eeprom.begin();

    if (result == 0)
    {
        Log.infoln("M24M02 found");

        initialized = true;

        LedController::getInstance().blink(LedSuccess, LedEEProm);
    }
    else
    {
        Log.infoln("M24M02 init failed : %d", result);

        LedController::getInstance().blink(LedError, LedEEProm);
    }

    return initialized;
}

bool M24M02Hal::read(const uint32_t addr, uint8_t* values, const size_t size)
{
    const auto result = eeprom.read(addr, values, size);

    if (result == 0)
    {
        Log.infoln("M24M02 read OK of %d bytes at address: %X", size, addr);

        return true;
    }

    Log.errorln("M24M02 read KO of %d bytes at address: %X. Error: %d", size, addr, result);

    return false;
}

uint8_t M24M02Hal::read(const uint32_t addr)
{
    const auto value = eeprom.read(addr);

    if (value >= 0)
    {
        Log.infoln("M24M02 read OK of %d at address: %X", value, addr);

        return true;
    }

    Log.errorln("M24M02 read KO at address: %X. Error: %d", addr, value);

    return false;
}

bool M24M02Hal::write(const uint32_t addr, uint8_t* values, const size_t size)
{
    const auto result = eeprom.write(addr, values, size);

    if (result == 0)
    {
        Log.infoln("M24M02 write OK of %d bytes to address: %X", size, addr, result);

        return true;
    }

    Log.errorln("M24M02 write KO of %d bytes to address: %X. Error: %d", size, addr, result);

    return false;
}

bool M24M02Hal::write(const uint32_t addr, const uint8_t value)
{
    const auto result = eeprom.write(addr, value);

    if (result == 0)
    {
        Log.infoln("M24M02 write OK of %d to address: %X", value, addr, result);

        return true;
    }

    Log.errorln("M24M02 write KO of %d to address: %X. Error: %d", value, addr, result);

    return result == 0;
}

uint8_t M24M02Hal::update(const uint32_t addr, const uint8_t value)
{
    const auto result = eeprom.update(addr, value);

    if (result == 0)
    {
        Log.infoln("M24M02 update OK of %d to address: %X", value, addr, result);

        return true;
    }

    Log.errorln("M24M02 update KO of %d to address: %X. Error: %d", value, addr, result);

    return false;
}

#include "hal/Tca9555Hal.hpp"

#include <ArduinoLog.h>

Tca9555Hal::Tca9555Hal(uint8_t address) : tca(address)
{
}

bool Tca9555Hal::begin()
{
    if (initialized)
    {
        return true;
    }

    Log.infoln("Init TCA9555 address %X", tca.getAddress());

    initialized = tca.begin() && tca.lastError() == TCA9555_OK;

    if (initialized)
    {
        Log.infoln("Init TCA9555 %X OK", tca.getAddress());
    }
    else
    {
        Log.warningln("Init TCA9555 %X failed", tca.getAddress());
    }

    return initialized;
}

bool Tca9555Hal::setMode(uint8_t pin, PinMode mode)
{
    Log.infoln("TCA9555 %X pin mode %d to %d", tca.getAddress(), pin, mode);

    if (!begin())
    {
        Log.infoln("TCA9555 %X pin mode %d to %d KO", tca.getAddress(), pin, mode);

        return false;
    }

    initialized = tca.pinMode1(pin, mode) && tca.lastError() == TCA9555_OK;

    if (initialized)
    {
        Log.warningln("TCA9555 %X pin mode %d to %d KO", tca.getAddress(), pin, mode);
    }
    else
    {
        Log.infoln("TCA9555 %X pin mode %d to %d OK", tca.getAddress(), pin, mode);
    }

    return initialized;
}

bool Tca9555Hal::write(uint8_t pin, bool value)
{
    Log.infoln("TCA9555 %X pin write %d to %d", tca.getAddress(), pin, value);

    if (!begin())
    {
        Log.infoln("TCA9555 %X pin write %d to %d KO", tca.getAddress(), pin, value);

        return false;
    }

    initialized = tca.write1(pin, value) && tca.lastError() == TCA9555_OK;

    if (initialized)
    {
        Log.warningln("TCA9555 %X pin write %d to %d KO", tca.getAddress(), pin, value);
    }
    else
    {
        Log.infoln("TCA9555 %X pin write %d to %d OK", tca.getAddress(), pin, value);
    }

    return initialized;
}

PinStatus Tca9555Hal::read(uint8_t pin)
{
    Log.infoln("TCA9555 %X pin read %d", tca.getAddress(), pin);

    if (!begin())
    {
        Log.infoln("TCA9555 %X pin read %d KO", tca.getAddress(), pin);

        return LOW;
    }

    const auto result = tca.read1(pin);
    initialized = tca.lastError() == TCA9555_OK;

    if (initialized)
    {
        Log.warningln("TCA9555 %X pin read %d = %d KO", tca.getAddress(), pin, result);
    }
    else
    {
        Log.infoln("TCA9555 %X pin read %d = %d OK", tca.getAddress(), pin, result);
    }

    return static_cast<PinStatus>(result);
}

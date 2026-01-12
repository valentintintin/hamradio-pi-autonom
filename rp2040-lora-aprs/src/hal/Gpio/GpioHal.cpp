#include "hal/Gpio/GpioHal.hpp"

#include <ArduinoLog.h>

GpioHal::GpioHal(const uint8_t pin, const PinMode mode, const bool inverted, const bool useI2C, GpioHal* gpioToLow) :
    pin(pin), mode(mode), inverted(inverted), useI2C(useI2C), gpioToLow(gpioToLow)
{
}

bool GpioHal::begin()
{
    Log.infoln("Pin %d set mode to %d", pin, mode);
    auto result = doBegin();

    if (gpioToLow)
    {
        result &= gpioToLow->doBegin();
    }

    return result;
}

PinStatus GpioHal::set(const PinStatus state)
{
    bool result = false;

    if (!gpioToLow)
    {
        if (inverted)
        {
            result = doSet(state == HIGH ? LOW : HIGH);
        }
        else
        {
            result = doSet(state);
        }
    }
    else
    {
        // TODO fix le mode inversé, pas sûr qu'il fonctionne ici
        if (state == HIGH)
        {
            if (inverted)
            {
                result = doSet(LOW) && gpioToLow->set(HIGH);
            }
            else
            {
                result = doSet(HIGH) && gpioToLow->set(LOW);
            }
        }
        else if (state == LOW)
        {
            if (inverted)
            {
                result = doSet(HIGH) && gpioToLow->set(LOW);
            }
            else
            {
                result = doSet(LOW) && gpioToLow->set(HIGH);
            }
        }
    }

    if (result)
    {
        pinStatus = state;
    }

    return pinStatus;
}

PinStatus GpioHal::toggle()
{
    const auto status = pinStatus == HIGH ? LOW : HIGH;
    Log.infoln("Pin %d toggled to %T", pin, status);
    return set(status);
}

PinStatus GpioHal::read()
{
    pinStatus = doRead();

    if (inverted)
    {
        pinStatus = pinStatus == HIGH ? LOW : HIGH;
    }

    Log.infoln("Pin %d read : %T", pin, pinStatus);
    return pinStatus;
}

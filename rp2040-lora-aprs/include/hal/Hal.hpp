#pragma once

#include <ArduinoLog.h>
#include "controllers/LedController.hpp"

class Hal
{
public:
    virtual bool begin();

    bool isInitialized() const
    {
        return initialized;
    }

    virtual ~Hal() = default;
protected:
    bool initialized = false;

    virtual bool doBegin() = 0;
};

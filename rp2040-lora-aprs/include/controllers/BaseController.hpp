#pragma once

#include <FreeRTOS.h>

class BaseController
{
public:
    virtual bool begin() = 0;
    virtual ~BaseController() = default;
};

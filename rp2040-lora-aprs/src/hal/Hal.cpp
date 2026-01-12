#include "hal/Hal.hpp"

bool Hal::begin()
{
    initialized = doBegin();
    return initialized;
}
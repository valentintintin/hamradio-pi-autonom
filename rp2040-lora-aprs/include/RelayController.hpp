#pragma once
#include <map>
#include <Arduino.h>
#include <memory>

#include "BaseController.hpp"
#include "GpioHal.hpp"

#define MAX_RELAYS 4

struct RelayCommand {
    enum { CAMERA, WIFI, LINUX, MESH } id;
    bool state;
};

class RelayController final : public BaseController {
public:
    bool addRelay(uint8_t id, GpioHal* gpio);;
    bool begin() override;
    bool processCommand() override;
private:
    GpioHal* relays[MAX_RELAYS];
    uint8_t nbRelays = 0;
};
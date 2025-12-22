#pragma once
#include <Arduino.h>

#include "BaseController.hpp"
#include "hal/GpioHal.hpp"
#include "config.h"

#include <queue.h>

struct RelayCommand
{
    uint8_t id;
    bool state;
};

class RelayController : public BaseController
{
public:
    static RelayController& getInstance()
    {
        static RelayController instance;
        return instance;
    }

    bool begin() override;

    uint8_t loadRelayFromSettings();
    bool changeState(uint8_t id, bool state) const;

    static void task(void* pvParameters);

private:
    QueueHandle_t queue;

    GpioHal* relays[MAX_GPIO_USED]{};
    uint8_t nbRelays = 0;

    RelayController();
};

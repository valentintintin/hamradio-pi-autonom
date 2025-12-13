#pragma once
#include <Arduino.h>

#include "BaseController.hpp"
#include "GpioHal.hpp"
#include "config.h"

struct RelayCommand {
    uint8_t id;
    bool state;
};

class RelayController final : public BaseController {
public:
    explicit RelayController(QueueHandle_t *queue);

    int8_t addRelay(GpioHal* gpio);
    bool begin() override;

    static void task(void *pvParameters);
private:
    GpioHal* relays[MAX_GPIO_USED];
    uint8_t nbRelays = 0;
};
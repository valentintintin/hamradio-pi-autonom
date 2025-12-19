#include "controllers/RelayController.hpp"

#include <ArduinoLog.h>

RelayController::RelayController(QueueHandle_t *queue) : BaseController(queue), relays{} {
}

int8_t RelayController::addRelay(GpioHal *gpio) {
    if (nbRelays >= MAX_GPIO_USED) {
        Log.errorln(F("Can't add relay, no more space"));
        return -1;
    }

    Log.infoln(F("Relay pin %d added with id %d"), gpio->pin, nbRelays);
    relays[nbRelays] = gpio;

    return nbRelays++;
}

bool RelayController::begin() {
    Log.infoln(F("Relay begin"));

    bool result = false;

    for (const auto gpio : relays) {
        if (gpio == nullptr) {
            continue;
        }

        result &= gpio->init();
    }

    xTaskCreate(task, "RelayTask", configMINIMAL_STACK_SIZE, this, tskIDLE_PRIORITY, nullptr);

    return result;
}

bool RelayController::changeState(const uint8_t id, const bool state) const {
    const RelayCommand command = {
        .id = id,
        .state = state
    };
    return xQueueSend(*queue, &command, 0) == pdTRUE;
}

void RelayController::task(void *pvParameters) {
    const auto* ctrl = static_cast<RelayController*>(pvParameters);
    RelayCommand command;

    Log.infoln(F("Relay task started"));

    while (true) {
        if (xQueueReceive(*ctrl->queue, &command, portMAX_DELAY) == pdTRUE) {
            Log.infoln(F("RelayController receive message for id %d"), command.id);

            if (command.id >= ctrl->nbRelays) {
                Log.errorln(F("No relay for id %d"), command.id);
                continue;
            }

            const auto relay = ctrl->relays[command.id];
            relay->set(command.state);

            Log.infoln(F("RelayController message received done for id %d"), command.id);
        }
    }
}

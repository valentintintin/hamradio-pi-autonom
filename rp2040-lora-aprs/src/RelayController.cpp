#include "../include/RelayController.hpp"

bool RelayController::addRelay(uint8_t id, GpioHal *gpio) {
    if (nbRelays >= MAX_RELAYS) {
        // TODO log
        return false;
    }

    relays[nbRelays++] = gpio;

    return true;
}

bool RelayController::begin() {
    bool result = false;

    for (const auto gpio : relays) {
        if (gpio == nullptr) {
            continue;
        }

        result &= gpio->init();
    }

    xTaskCreate(task, "RelayTask", 1024, this, 1, nullptr);

    return result;
}

bool RelayController::processCommand() {
}

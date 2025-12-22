#include "controllers/RelayController.hpp"

#include <ArduinoLog.h>

#include "SettingsManager.hpp"
#include "hal/PicoGpioHal.hpp"

RelayController::RelayController() : relays{}
{
    queue = xQueueCreate(2, sizeof(RelayCommand));

    if (queue == nullptr)
    {
        Log.errorln(F("Relay Queue creation failed"));
    }
}

bool RelayController::begin()
{
    Log.infoln(F("Relay begin"));

    const bool result = loadRelayFromSettings() > 0;

    if (xTaskCreate(task, "RelayTask", configMINIMAL_STACK_SIZE, this, tskIDLE_PRIORITY, nullptr) != pdPASS)
    {
        Log.errorln("Relay task creation failed");
    }

    return result;
}

uint8_t RelayController::loadRelayFromSettings()
{
    for (const auto pin : SettingsManager::getSettings().pins)
    {
        if (pin.i2cAddress == 0)
        {
            const auto gpio = new PicoGpioHal(pin.pin, pin.mode, pin.inverted);

            relays[nbRelays] = gpio;

            Log.infoln(F("Relay pin %d added with id %d"), gpio->pin, nbRelays);

            if (!gpio->init())
            {
                Log.warningln(F("Relay pin %d added with id %d failed to init"), gpio->pin, nbRelays);
            }

            nbRelays++;

            if (nbRelays >= MAX_GPIO_USED)
            {
                Log.warningln("Max relay reached %d", nbRelays);
                break;
            }
        }
    }

    return nbRelays;
}

bool RelayController::changeState(const uint8_t id, const bool state) const
{
    const RelayCommand command = {
        .id = id,
        .state = state
    };

    if (xQueueSend(queue, &command, 0) != pdTRUE)
    {
        Log.warningln("Relay queue send failed for id %d", id);

        return false;
    }

    Log.infoln("Relay queue send OK for id %d", id);

    return true;
}

void RelayController::task(void* pvParameters)
{
    Log.infoln(F("Relay task started"));

    const auto* ctrl = static_cast<RelayController*>(pvParameters);
    RelayCommand command;

    while (true)
    {
        if (xQueueReceive(ctrl->queue, &command, portMAX_DELAY) == pdTRUE)
        {
            Log.infoln(F("RelayController receive message for id %d"), command.id);

            if (command.id >= ctrl->nbRelays)
            {
                Log.errorln(F("No relay for id %d"), command.id);
                continue;
            }

            const auto relay = ctrl->relays[command.id];
            relay->set(command.state);

            Log.infoln(F("RelayController message received done for id %d"), command.id);
        }
    }
}

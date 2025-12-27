#include "controllers/RelayController.hpp"

#include <ArduinoLog.h>

#include "SettingsManager.hpp"
#include "controllers/LedController.hpp"
#include "hal/I2CMasterHal.hpp"
#include "hal/PicoGpioHal.hpp"

RelayController::RelayController()
{
    queue = xQueueCreate(RELAY_MAX_COMMAND, sizeof(RelayCommand));

    if (queue == nullptr)
    {
        Log.errorln("Relay Queue creation failed");

        LedController::getInstance().blink(Error, FreeRtos);
    }
}

bool RelayController::begin()
{
    Log.infoln("Relay begin");

    const bool result = loadRelayFromSettings() > 0;

    if (result)
    {
        if (xTaskCreate(task, "RelayTask", configMINIMAL_STACK_SIZE, this, tskIDLE_PRIORITY, nullptr) != pdPASS)
        {
            Log.errorln("Relay task creation failed");

            LedController::getInstance().blink(Error, FreeRtos);

            return false;
        }

        LedController::getInstance().blink(Success, Relay);
    }

    return result;
}

uint8_t RelayController::loadRelayFromSettings()
{
    for (const auto pin : SettingsManager::getSettings().pins)
    {
        if (pin.i2cAddress == 0 && strlen(pin.name) > 0)
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

                LedController::getInstance().blink(Error, Relay);

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

        LedController::getInstance().blink(Error, Relay);

        return false;
    }

    Log.infoln("Relay queue send OK for id %d", id);

    return true;
}

void RelayController::task(void* pvParameters)
{
    const auto* ctrl = static_cast<RelayController*>(pvParameters);
    RelayCommand command;

    while (true)
    {
        if (xQueueReceive(ctrl->queue, &command, portMAX_DELAY) == pdTRUE)
        {
            Log.infoln("RelayController receive message for id %d", command.id);

            if (command.id >= ctrl->nbRelays)
            {
                Log.errorln("No relay for id %d", command.id);

                LedController::getInstance().blink(Error, Relay);

                continue;
            }

            const auto relay = ctrl->relays[command.id];

            if (relay->useI2C)
            {
                if (!I2CMasterHal::takeSemaphore())
                {
                    Log.warningln("RelayController can not set relay I2C %d, can not have semaphore", command.id);

                    LedController::getInstance().blink(Error, I2C);

                    continue;
                }
            }

            relay->set(command.state);

            if (relay->useI2C)
            {
                I2CMasterHal::releaseSemaphore();
            }

            Log.infoln("RelayController message received done for id %d", command.id);

            LedController::getInstance().blink(Success, Relay);
        }
    }
}

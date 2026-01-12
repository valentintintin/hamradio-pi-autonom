#include "controllers/RelayController.hpp"

#include <ArduinoLog.h>

#include "SettingsManager.hpp"
#include "controllers/LedController.hpp"
#include "hal/I2CMasterHal.hpp"
#include "hal/Gpio/PicoGpioHal.hpp"
#include "hal/Gpio/Tca9555GpioHal.hpp"

RelayController::RelayController()
{
    queue = xQueueCreate(RELAY_MAX_COMMAND, sizeof(RelayCommand));

    if (queue == nullptr)
    {
        Log.errorln("Relay Queue creation failed");

        LedController::getInstance().blink(LedError, LedFreeRtos);
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

            LedController::getInstance().blink(LedError, LedFreeRtos);

            return false;
        }

        LedController::getInstance().blink(LedSuccess, LedRelay);
    }

    return result;
}

uint8_t RelayController::loadRelayFromSettings()
{
    for (const auto gpioConfig : SettingsManager::getSettings().gpio)
    {
        if (!gpioConfig.enabled)
        {
            continue;
        }

        GpioHal *gpio;
        GpioHal *gpioToLow = nullptr;

        if (gpioConfig.i2cAddress == 0)
        {
            if (gpioConfig.pinToLow > 0)
            {
                gpioToLow = new PicoGpioHal(gpioConfig.pinToLow, gpioConfig.mode, gpioConfig.inverted);
            }
            gpio = new PicoGpioHal(gpioConfig.pin, gpioConfig.mode, gpioConfig.inverted, gpioToLow);
        }
        else
        {
            if (gpioConfig.pinToLow > 0)
            {
                gpioToLow = new Tca9555GpioHal(Tca9555Hal::getInstance(gpioConfig.i2cAddress), gpioConfig.pinToLow, gpioConfig.mode, gpioConfig.inverted);
            }

            gpio = new Tca9555GpioHal(Tca9555Hal::getInstance(gpioConfig.i2cAddress), gpioConfig.pin, gpioConfig.mode, gpioConfig.inverted, gpioToLow);
        }

        relays[nbRelays] = gpio;

        Log.infoln("Relay pin %d added with id %d", gpio->pin, nbRelays);

        if (!gpio->begin())
        {
            Log.warningln("Relay pin %d added with id %d failed to init", gpio->pin, nbRelays);
        }

        nbRelays++;

        if (nbRelays >= MAX_GPIO_USED)
        {
            Log.warningln("Max relay reached %d", nbRelays);

            LedController::getInstance().blink(LedError, LedRelay);

            break;
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

        LedController::getInstance().blink(LedError, LedRelay);

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

                LedController::getInstance().blink(LedError, LedRelay);

                continue;
            }

            const auto relay = ctrl->relays[command.id];

            if (relay->useI2C)
            {
                if (!I2CMasterHal::takeSemaphore())
                {
                    Log.warningln("RelayController can not set relay I2C %d, can not have semaphore", command.id);

                    LedController::getInstance().blink(LedError, LedI2C);

                    continue;
                }
            }

            relay->set(command.state ? HIGH : LOW);

            if (relay->useI2C)
            {
                I2CMasterHal::releaseSemaphore();
            }

            Log.infoln("RelayController message received done for id %d", command.id);

            LedController::getInstance().blink(LedSuccess, LedRelay);
        }
    }
}

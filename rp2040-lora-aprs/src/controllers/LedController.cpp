#include "controllers/LedController.hpp"

#include <ArduinoLog.h>

LedController::LedController()
{
    queue = xQueueCreate(LED_MAX_COMMAND, sizeof(LedUserCommand));

    if (queue == nullptr)
    {
        Log.errorln("Led Queue creation failed");

        getInstance().blink(LedError, LedFreeRtos);
    }
}

bool LedController::begin()
{
    if (xTaskCreate(task, "LedTask", configMINIMAL_STACK_SIZE, this, tskIDLE_PRIORITY, nullptr) != pdPASS)
    {
        Log.errorln("Relay task creation failed");

        getInstance().blink(LedError, LedFreeRtos);

        return false;
    }

    return true;
}

void LedController::blink(const LedState state, const LedOrigin origin) const
{
    const LedCommand command = {
        .state = state,
        .origin = origin
    };

    if (origin == LedWatchdog && uxQueueMessagesWaiting(queue) > 0)
    {
        return;
    }

    if (xQueueSend(queue, &command, 0) != pdTRUE)
    {
        Log.warningln("Led queue send failed");
    }
}

void LedController::task(void* pvParameters)
{
    const auto* ctrl = static_cast<LedController*>(pvParameters);
    LedCommand command;

    while (true)
    {
        if (xQueueReceive(ctrl->queue, &command, portMAX_DELAY) == pdTRUE)
        {
            for (uint8_t i = 0; i < command.origin; i++)
            {
                digitalWrite(LED_BUILTIN, HIGH);
                vTaskDelay(pdMS_TO_TICKS(command.state));
                digitalWrite(LED_BUILTIN, LOW);
                vTaskDelay(pdMS_TO_TICKS(command.state));
            }

            vTaskDelay(LED_DELAY);
        }
    }
}

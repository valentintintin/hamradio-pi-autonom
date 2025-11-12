#include <Arduino.h>
#include <FreeRTOS.h>
#include <task.h>

#include "config.h"
#include <RadioLib.h>

[[noreturn]] void heartBeatTask(void *pvParameters) {
    pinMode(LED_BUILTIN, OUTPUT);
    for (;;) {
        digitalWrite(LED_BUILTIN, HIGH);
        vTaskDelay(500 / portTICK_PERIOD_MS);
        digitalWrite(LED_BUILTIN, LOW);
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}

void setup() {
    xTaskCreate(heartBeatTask, "HeartBeat", 128, nullptr, 1, nullptr);

    vTaskStartScheduler();
}

void loop() {
}

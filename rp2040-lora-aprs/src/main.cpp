#include <Arduino.h>
#include <FreeRTOS.h>
#include <task.h>

#include "config.h"

void BlinkTask(void *pvParameters) {
    pinMode(25, OUTPUT);
    for (;;) {
        digitalWrite(25, HIGH);
        vTaskDelay(500 / portTICK_PERIOD_MS);
        digitalWrite(25, LOW);
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}

void setup() {
    xTaskCreate(BlinkTask, "Blink", 128, NULL, 1, NULL);

    vTaskStartScheduler();
}

void loop() {
    // loop vide si tout est géré par FreeRTOS
}

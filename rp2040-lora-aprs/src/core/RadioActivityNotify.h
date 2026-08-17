#pragma once

#include <FreeRTOS.h>
#include <task.h>

// Réveillés depuis le contexte IRQ DIO1 sur activité radio (RX/TX), évite le polling en vTaskDelay(1).
// Handle nullptr toléré : xTaskNotifyGive/vTaskNotifyGiveFromISR dessus est un no-op silencieux avant le démarrage de la tâche.
extern TaskHandle_t g_aprs_task_handle;
extern TaskHandle_t g_mesh_task_handle;

#pragma once

#include <cstdint>

// ============================================================================
// FreeRTOS.h — shim natif. Surface réellement utilisée par src/ :
// xTaskCreate/vTaskDelay/vTaskDelete, mutex/mutex récursif, pdMS_TO_TICKS,
// portMAX_DELAY, et les notifications de tâche (xTaskGetCurrentTaskHandle/
// ulTaskNotifyTake/xTaskNotifyGive/vTaskNotifyGiveFromISR — réveil des tâches
// radio depuis l'IRQ DIO1 simulée, cf. core/RadioActivityNotify.h). Pas de
// vraies queues FreeRTOS (non utilisées par le projet). Implémentation
// (threads/mutex/condition_variable std::) dans native/compat/freertos_compat.cpp.
//
// portYIELD_FROM_ISR : no-op ici — un vrai FreeRTOS doit demander
// explicitement un changement de contexte en sortie d'ISR si elle a réveillé
// une tâche plus prioritaire ; en natif chaque tâche est un vrai thread OS
// avec préemption réelle du noyau Linux, ce réordonnancement n'a pas de sens
// et n'est jamais nécessaire.
// ============================================================================

typedef uint32_t TickType_t;
typedef int BaseType_t;

#define pdTRUE  1
#define pdFALSE 0
#define pdMS_TO_TICKS(ms) ((TickType_t)(ms))
#define portMAX_DELAY ((TickType_t)0xFFFFFFFFUL)
#define portTICK_PERIOD_MS 1
#define portYIELD_FROM_ISR(xHigherPriorityTaskWoken) ((void)(xHigherPriorityTaskWoken))

typedef void* TaskHandle_t;

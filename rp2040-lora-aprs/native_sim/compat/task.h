#pragma once

#include "FreeRTOS.h"

// Chaque tâche (déjà une boucle `for(;;)`) devient un std::thread détaché —
// cf. native/compat/freertos_compat.cpp.
typedef void (*TaskFunction_t)(void*);

BaseType_t xTaskCreate(TaskFunction_t fn, const char* name, uint32_t stackWords,
                       void* params, uint32_t priority, TaskHandle_t* handle);

void vTaskDelay(TickType_t ticks);

// Seul task_watchdog.cpp l'utilise, toujours avec nullptr (auto-suppression) :
// implémenté via une exception interne attrapée au point d'entrée du thread.
void vTaskDelete(TaskHandle_t handle);

// ============================================================================
// Notifications de tâche — réveil des tâches radio (APRS/mesh/weather) depuis
// l'IRQ DIO1 simulée (cf. core/RadioActivityNotify.h et
// variant_native/AprsRadioHwSim.cpp) au lieu d'un polling en vTaskDelay(1).
// Mêmes noms/signatures que la vraie API FreeRTOS (task_notify.h) : le code
// appelant ne sait pas laquelle des deux implémentations il utilise.
// ============================================================================

// Handle de la tâche courante (celle qui appelle) — nullptr si appelée hors
// d'une tâche créée par xTaskCreate.
TaskHandle_t xTaskGetCurrentTaskHandle();

// Bloque jusqu'à notification ou expiration de xTicksToWait (portMAX_DELAY =
// indéfiniment). Retourne le compteur de notifications reçues (remis à zéro
// si xClearCountOnExit, ce qui est le seul usage de ce projet — un simple
// "réveil binaire").
uint32_t ulTaskNotifyTake(BaseType_t xClearCountOnExit, TickType_t xTicksToWait);

// Notifie depuis un contexte NON-interruption (ex: producteur qui vient
// d'enfiler un paquet TX).
BaseType_t xTaskNotifyGive(TaskHandle_t xTaskToNotify);

// Notifie depuis le contexte "interruption" simulé (callback RadioLib) —
// *pxHigherPriorityTaskWoken n'est jamais utilisé ici (portYIELD_FROM_ISR est
// un no-op en natif, cf. FreeRTOS.h) mais reste renseigné pour coller à la
// vraie signature FreeRTOS.
void vTaskNotifyGiveFromISR(TaskHandle_t xTaskToNotify, BaseType_t* pxHigherPriorityTaskWoken);

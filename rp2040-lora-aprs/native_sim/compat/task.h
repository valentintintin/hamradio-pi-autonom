#pragma once

#include "FreeRTOS.h"

typedef void (*TaskFunction_t)(void*);

BaseType_t xTaskCreate(TaskFunction_t fn, const char* name, uint32_t stackWords,
                       void* params, uint32_t priority, TaskHandle_t* handle);

void vTaskDelay(TickType_t ticks);

// vTaskDelete: seul usage réel est vTaskDelete(nullptr) (auto-suppression).
void vTaskDelete(TaskHandle_t handle);

TaskHandle_t xTaskGetCurrentTaskHandle();

uint32_t ulTaskNotifyTake(BaseType_t xClearCountOnExit, TickType_t xTicksToWait);

BaseType_t xTaskNotifyGive(TaskHandle_t xTaskToNotify);

void vTaskNotifyGiveFromISR(TaskHandle_t xTaskToNotify, BaseType_t* pxHigherPriorityTaskWoken);

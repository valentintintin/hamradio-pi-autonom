#pragma once

#include <cstdint>

typedef uint32_t TickType_t;
typedef int BaseType_t;

#define pdTRUE  1
#define pdFALSE 0
#define pdMS_TO_TICKS(ms) ((TickType_t)(ms))
#define portMAX_DELAY ((TickType_t)0xFFFFFFFFUL)
#define portTICK_PERIOD_MS 1
#define portYIELD_FROM_ISR(xHigherPriorityTaskWoken) ((void)(xHigherPriorityTaskWoken))

typedef void* TaskHandle_t;

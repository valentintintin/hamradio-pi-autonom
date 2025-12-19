#pragma once
#include <ctime>

#include <FreeRTOS.h>
#include <timers.h>

void setSlowClock();
void setTimeToInternalRtc(time_t epoch);

void rebootTask(TimerHandle_t xTimer);
void dfuTask(TimerHandle_t xTimer);
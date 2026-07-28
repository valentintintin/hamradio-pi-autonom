#include "RadioActivityNotify.h"

TaskHandle_t g_aprs_task_handle = nullptr;
TaskHandle_t g_mesh_task_handle = nullptr;

void notifyRadioActivityFromISR() {
  BaseType_t woken_aprs = pdFALSE;
  BaseType_t woken_mesh = pdFALSE;

  if (g_aprs_task_handle) {
    vTaskNotifyGiveFromISR(g_aprs_task_handle, &woken_aprs);
  }
  if (g_mesh_task_handle) {
    vTaskNotifyGiveFromISR(g_mesh_task_handle, &woken_mesh);
  }

  portYIELD_FROM_ISR(woken_aprs || woken_mesh);
}

#include "tasks.h"
#include "core/Log.h"
#include "core/RadioActivityNotify.h"
#include "../aprs/AprsDispatcher.h"
#include "task_heartbeat.h"

extern AprsDispatcher aprs_dispatcher;

#define TAG "APRS-TSK"

// Plafond de sommeil : garantit un heartbeat() régulier même sans activité
// radio, bien en dessous du seuil "bloquée" du watchdog (90s, cf.
// task_watchdog.cpp) — sinon un silence radio prolongé (normal en LoRa)
// ferait croire à tort la tâche figée.
#define APRS_TASK_MAX_WAIT_MS 60000

void taskAprsLoop(void* params) {
  (void)params;
  LOG_D(TAG, "Task démarrée");
  g_aprs_task_handle = xTaskGetCurrentTaskHandle();

  for (;;) {
    heartbeat(HB_APRS);
    aprs_dispatcher.loop();

    uint32_t wait_ms = aprs_dispatcher.msUntilNextAction(millis());
    if (wait_ms > APRS_TASK_MAX_WAIT_MS) {
      wait_ms = APRS_TASK_MAX_WAIT_MS;
    }
    ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(wait_ms));
  }
}

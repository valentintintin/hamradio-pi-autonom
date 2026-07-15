#include "tasks.h"
#include "core/Log.h"
#include "../aprs/AprsDispatcher.h"
#include "task_heartbeat.h"

// ============================================================================
// Task APRS — appelle le dispatcher APRS en continu
// ============================================================================

extern AprsDispatcher aprs_dispatcher;

#define TAG "APRS-TSK"

void taskAprsLoop(void* params) {
  (void)params;
  LOG_D(TAG, "Task démarrée");

  for (;;) {
    heartbeat(HB_APRS);
    aprs_dispatcher.loop();
    vTaskDelay(pdMS_TO_TICKS(1));
  }
}

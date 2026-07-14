#include "tasks.h"
#include "config/Log.h"
#include "../aprs/AprsDispatcher.h"
#include "TaskHeartbeat.h"

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

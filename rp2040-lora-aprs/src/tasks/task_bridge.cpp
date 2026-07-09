#include "tasks.h"
#include "config/Log.h"
#include "../bridge/MeshAprsBridge.h"

// ============================================================================
// Task Bridge — passerelle mesh ↔ APRS
// ============================================================================

extern MeshAprsBridge mesh_aprs_bridge;

#define TAG "BRIDGE-T"

void taskAprsBridge(void* params) {
  (void)params;
  LOG_D(TAG, "Task démarrée");

  for (;;) {
    mesh_aprs_bridge.loop();
    vTaskDelay(pdMS_TO_TICKS(1000)); // check toutes les secondes
  }
}

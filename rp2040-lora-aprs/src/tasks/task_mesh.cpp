#include "tasks.h"
#include "target.h"
#include "core/Log.h"
#include "core/RadioActivityNotify.h"
#include "../mesh/MeshcoreRepeater.h"
#include "task_heartbeat.h"

extern MyMesh the_mesh;

#define TAG "MESH-TSK"

// Cf. commentaire équivalent dans task_aprs.cpp : plafond bien en dessous du
// seuil "bloquée" du watchdog (90s).
#define MESH_TASK_MAX_WAIT_MS 60000

void taskMeshLoop(void* params) {
  (void)params;
  LOG_D(TAG, "Task démarrée");
  g_mesh_task_handle = xTaskGetCurrentTaskHandle();

  for (;;) {
    heartbeat(HB_MESH);

    the_mesh.loop();

    // Le tick peut être manqué vu qu'on dort entre deux appels — sans gravité,
    // on a une vraie RTC en secours.
    rtc_clock.tick();
    sensors.loop();

    ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(MESH_TASK_MAX_WAIT_MS));
  }
}

#include "tasks.h"
#include "target.h"
#include "config/Log.h"
#include "../mesh/MeshcoreRepeater.h"

// ============================================================================
// Task MeshCore — appelle le dispatcher/mesh loop en continu
// ============================================================================

extern MyMesh the_mesh;

#define TAG "MESH-TSK"

void taskMeshLoop(void* params) {
  (void)params;
  LOG_D(TAG, "Task démarrée");

  for (;;) {
    the_mesh.loop();

    // Horloge de secours (millis()) + capteurs MeshCore (GPS, etc.) : comme
    // dans le loop() de référence des exemples MeshCore, à appeler à chaque
    // itération.
    rtc_clock.tick();
    sensors.loop();

    // Yield bref — le dispatcher a besoin de tourner souvent
    // pour ne pas rater de paquets RX
    vTaskDelay(pdMS_TO_TICKS(1));
  }
}

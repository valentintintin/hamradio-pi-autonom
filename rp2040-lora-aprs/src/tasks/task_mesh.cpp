#include "tasks.h"
#include "target.h"
#include "core/Log.h"
#include "core/RadioActivityNotify.h"
#include "../mesh/MeshcoreRepeater.h"
#include "task_heartbeat.h"

// ============================================================================
// Task MeshCore — appelle le dispatcher/mesh loop, puis dort jusqu'au
// prochain paquet radio réel (réveillé depuis l'IRQ DIO1 — cf.
// core/RadioActivityNotify.h) ou MESH_TASK_MAX_WAIT_MS au plus, au lieu d'un
// polling fixe en vTaskDelay(1).
//
// Contrairement à AprsDispatcher (projet), le Dispatcher/Mesh interne de
// MeshCore (vendoré, lib/MeshCore/src/{Dispatcher,Mesh}.cpp) reste inchangé :
// sa logique de retransmission/ACK/flood-control gère déjà ses propres délais
// via millis() et tolère d'être rappelée en retard (jusqu'à
// MESH_TASK_MAX_WAIT_MS) exactement comme un appel périodique classique — on
// ne fait que remplacer l'attente fixe entre deux appels par une attente
// réveillable plus tôt sur activité radio réelle.
// ============================================================================

extern MyMesh the_mesh;

#define TAG "MESH-TSK"

// Plafond de sécurité — cf. commentaire équivalent dans tasks/task_aprs.cpp ;
// nettement en dessous du seuil "tâche bloquée" du watchdog (HB_RADIO_STALE_MS
// = 90s, cf. tasks/task_watchdog.cpp).
#define MESH_TASK_MAX_WAIT_MS 60000

void taskMeshLoop(void* params) {
  (void)params;
  LOG_D(TAG, "Task démarrée");
  g_mesh_task_handle = xTaskGetCurrentTaskHandle();

  for (;;) {
    heartbeat(HB_MESH);

    the_mesh.loop();

    // Horloge de secours (millis()) + capteurs MeshCore (GPS, etc.) : comme
    // dans le loop() de référence des exemples MeshCore, à appeler à chaque
    // itération.
    rtc_clock.tick(); // Vu qu'on dort, il est fort possible que le tick ne se fasse pas correctement (pas grave on a une rtc)
    sensors.loop();

    ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(MESH_TASK_MAX_WAIT_MS));
  }
}

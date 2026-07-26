// ============================================================================
// Task SSTV — tâche dédiée à l'émission CW+SSTV
//
// Découplée du CLI/APRS : "image send" (cli/CommandHandler.cpp) ne fait
// qu'armer une demande (SstvTransmitter::requestTransmit()) et rend la main
// immédiatement, y compris quand la commande arrive par APRS/mesh (qui tient
// le mutex partagé CommandHandler/AprsEngine pendant toute la durée de
// l'appel). L'émission elle-même (1-2 minutes selon le mode SSTV) tourne ici,
// hors de ce mutex, pour ne jamais geler le reste du CLI/APRS/mesh pendant ce
// temps (cf. aprs/SstvTransmitter.h et le plan associé).
// ============================================================================

#include "tasks.h"
#include "core/Log.h"
#include "aprs/SstvTransmitter.h"

extern SstvTransmitter sstv_transmitter;

#define TASK_SSTV_POLL_MS 200

void taskSstv(void* params) {
  (void)params;
  LOG_D("SSTV", "Task démarrée");

  for (;;) {
    if (sstv_transmitter.consumeTransmitRequest()) {
      sstv_transmitter.transmit();
    }
    vTaskDelay(pdMS_TO_TICKS(TASK_SSTV_POLL_MS));
  }
}

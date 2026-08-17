// Émission (1-2 min) découplée dans sa propre tâche pour ne jamais tenir le
// mutex partagé CommandHandler/AprsEngine pendant tout ce temps.

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

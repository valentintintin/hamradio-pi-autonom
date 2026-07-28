#include "tasks.h"
#include "core/Log.h"
#include "core/RadioActivityNotify.h"
#include "../aprs/AprsDispatcher.h"
#include "task_heartbeat.h"

// ============================================================================
// Task APRS — appelle le dispatcher APRS, puis dort jusqu'au prochain
// événement radio réel (RX ou TX terminé, réveillé depuis l'IRQ DIO1 — cf.
// core/RadioActivityNotify.h) ou jusqu'à ce qu'une action planifiée devienne
// due (retry CAD, paquet en attente — cf. AprsDispatcher::msUntilNextAction()),
// au lieu d'un polling fixe en vTaskDelay(1).
// ============================================================================

extern AprsDispatcher aprs_dispatcher;

#define TAG "APRS-TSK"

// Plafond de sécurité — juste de quoi garantir un heartbeat() de temps en
// temps même quand msUntilNextAction() ne retourne rien de proche (queue
// vide, pas de paquet radio) ; nettement en dessous du seuil "tâche bloquée"
// du watchdog (HB_RADIO_STALE_MS = 90s, cf. tasks/task_watchdog.cpp) pour
// laisser de la marge. Sans ce plafond (silence radio prolongé, ce qui est la
// norme et non l'exception), la tâche dormirait indéfiniment et le watchdog
// finirait par la croire bloquée à tort.
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

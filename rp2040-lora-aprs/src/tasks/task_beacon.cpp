#include "tasks.h"
#include "config/Log.h"
#include "../aprs/AprsEngine.h"

// ============================================================================
// Task Beacon APRS — envoie position/status/telemetry périodiquement
// ============================================================================

extern AprsEngine aprs_engine;

#define TAG "BEACON"

// Intervalles (en ms)
#define BEACON_POSITION_INTERVAL     (60 * 60 * 1000)   // 1h
#define BEACON_STATUS_INTERVAL       (24 * 60 * 60 * 1000) // 24h
#define BEACON_TELEMETRY_INTERVAL    (15 * 60 * 1000)    // 15 min
#define BEACON_BOOT_DELAY            (90 * 1000)          // 90s après boot

void taskAprsBeacon(void* params) {
  (void)params;

  // Attendre après le boot (éviter les TX storms)
  vTaskDelay(pdMS_TO_TICKS(BEACON_BOOT_DELAY));
  LOG_D(TAG, "Task démarrée");

  // Première salve
  aprs_engine.sendPosition(APRS_COMMENT);
  vTaskDelay(pdMS_TO_TICKS(5000));
  aprs_engine.sendTelemetryParams();

  unsigned long last_position = millis();
  unsigned long last_status = millis();
  unsigned long last_telemetry = millis();

  for (;;) {
    unsigned long now = millis();

    if (now - last_position >= BEACON_POSITION_INTERVAL) {
      LOG_T(TAG, "TX position");
      aprs_engine.sendPosition(APRS_COMMENT);
      last_position = now;
    }

    if (now - last_telemetry >= BEACON_TELEMETRY_INTERVAL) {
      LOG_T(TAG, "TX telemetry");
      aprs_engine.sendTelemetry();
      last_telemetry = now;
    }

    if (now - last_status >= BEACON_STATUS_INTERVAL) {
      LOG_T(TAG, "TX status");
      aprs_engine.sendStatus(APRS_COMMENT);
      last_status = now;
    }

    // Check toutes les 10s — pas besoin de plus
    vTaskDelay(pdMS_TO_TICKS(10000));
  }
}

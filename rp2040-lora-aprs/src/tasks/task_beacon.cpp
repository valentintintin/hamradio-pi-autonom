#include "tasks.h"
#include "config/Log.h"
#include "config/Settings.h"
#include "../aprs/AprsEngine.h"
#include <Timer.h>

// ============================================================================
// Task Beacon APRS — envoie position/météo/telemetry/statut périodiquement
//
// Intervalles lus depuis `settings.aprs.interval.*` (modifiables à chaud via
// le CLI "set aprs.interval.xxx <ms>") :
//   - position  : 2x/jour
//   - telemetry : toutes les heures
//   - météo     : toutes les 15 min
//   - statut    : dès que l'état de la station change (sendStatusIfChanged),
//                 avec un envoi forcé au bout de intervalStatus_ms au cas où
//                 rien n'a changé (filet de sécurité "toujours entendu")
// ============================================================================

extern AprsEngine aprs_engine;
extern Settings settings;

#define TAG "BEACON"

#define BEACON_BOOT_DELAY   (90 * 1000)   // 90s après boot, éviter les TX storms
#define BEACON_CHECK_PERIOD (10 * 1000)   // fréquence de vérification des échéances

void taskAprsBeacon(void* params) {
  (void)params;

  // Attendre après le boot (éviter les TX storms)
  vTaskDelay(pdMS_TO_TICKS(BEACON_BOOT_DELAY));
  LOG_D(TAG, "Task démarrée");

  // Première salve
  aprs_engine.sendPosition(settings.aprs.comment);
  vTaskDelay(pdMS_TO_TICKS(5000));
  aprs_engine.sendTelemetryParams();
  vTaskDelay(pdMS_TO_TICKS(3000));
  aprs_engine.sendWeather();

  Timer position_timer(settings.aprs.intervalPosition_ms);
  Timer telemetry_timer(settings.aprs.intervalTelemetry_ms);
  Timer weather_timer(settings.aprs.intervalWeather_ms);

  for (;;) {
    // Resynchroniser l'intervalle si modifié à chaud ("set aprs.interval.xxx <ms>")
    position_timer.setInterval(settings.aprs.intervalPosition_ms, false);
    telemetry_timer.setInterval(settings.aprs.intervalTelemetry_ms, false);
    weather_timer.setInterval(settings.aprs.intervalWeather_ms, false);

    if (position_timer.hasExpired()) {
      LOG_T(TAG, "TX position");
      aprs_engine.sendPosition(settings.aprs.comment);
      position_timer.restart();
    }

    if (telemetry_timer.hasExpired()) {
      LOG_T(TAG, "TX telemetry");
      aprs_engine.sendTelemetry();
      telemetry_timer.restart();
    }

    if (weather_timer.hasExpired()) {
      LOG_T(TAG, "TX météo");
      aprs_engine.sendWeather();
      weather_timer.restart();
    }

    // Statut : envoyé dès que l'état change, sinon au plus tard toutes les
    // intervalStatus_ms (voir AprsEngine::sendStatusIfChanged)
    if (aprs_engine.sendStatusIfChanged(settings.aprs.intervalStatus_ms)) {
      LOG_T(TAG, "TX statut");
    }

    vTaskDelay(pdMS_TO_TICKS(BEACON_CHECK_PERIOD));
  }
}

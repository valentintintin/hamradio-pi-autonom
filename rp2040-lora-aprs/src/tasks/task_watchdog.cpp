#include "tasks.h"
#include "core/Log.h"
#include "config/Settings.h"
#include "hal/eeprom/EventLogHistory.h"
#include "task_heartbeat.h"
#include <hardware/structs/watchdog.h>
#include <Timer.h>

// ============================================================================
// Task Watchdog — arme et nourrit le watchdog matériel du RP2040, mais
// seulement si toutes les tâches surveillées ont donné signe de vie
// récemment (cf. TaskHeartbeat.h). Sans ça, un blocage d'une seule tâche
// (dispatcher APRS coincé sur un appel radio, par exemple) ne serait jamais
// détecté : cette tâche watchdog elle-même continuerait de tourner et de
// nourrir le chien indéfiniment, pendant que le reste de l'appareil serait
// mort pour de bon.
//
// Protection anti boucle de reboot : le nombre de reboots CONSÉCUTIFS causés
// par le watchdog est compté dans un registre "scratch" du périphérique
// watchdog lui-même (survit à un reset matériel, contrairement à la RAM
// normale que le runtime C réinitialise à chaque démarrage). Si trop de
// reboots watchdog s'enchaînent sans qu'un uptime "sain" ne soit atteint
// entre-temps, le chien n'est plus réarmé pour ce cycle de boot : mieux vaut
// un appareil qui tourne en mode dégradé (au moins joignable en CLI/APRS)
// qu'un appareil coincé dans une boucle de reboot infinie, sur un site
// difficile d'accès.
// ============================================================================

extern Settings settings;
extern EventLogHistory event_log;

#define TAG "WDT"
#define WATCHDOG_TIMEOUT_MS         8000              // proche du max matériel RP2040 (~8.3s)
#define WATCHDOG_FEED_PERIOD_MS     2000
#define WATCHDOG_HEALTHY_UPTIME_MS  (5UL * 60UL * 1000UL)  // 5 min sans blocage = boot considéré sain
#define WATCHDOG_MAX_CONSECUTIVE_REBOOTS 5

// Registre scratch utilisé comme compteur persistant de reboots watchdog consécutifs
#define WDT_SCRATCH_REBOOT_COUNT 0

static bool isTaskStale(HeartbeatTask task, uint32_t max_age_ms) {
  unsigned long last = g_heartbeat_ms[task];
  if (last == 0) {
    return false; // tâche pas encore démarrée (juste après boot) : pas encore de verdict
  }
  return (millis() - last) > max_age_ms;
}

// Âges max tolérés par tâche. Pour sensors/energy/weather, dont la période
// est configurable via settings, la marge est calculée sur la valeur
// réellement en vigueur plutôt qu'une constante figée (sinon un intervalle
// configuré plus long que la marge fixe déclencherait des faux positifs en
// continu).
static bool allTasksAlive() {
  if (isTaskStale(HB_MESH, 5000)) {
    LOG_E(TAG, "Tâche mesh bloquée");
    return false;
  }
  if (isTaskStale(HB_APRS, 5000)) {
    LOG_E(TAG, "Tâche APRS bloquée");
    return false;
  }
  if (isTaskStale(HB_BEACON, 30000)) {
    LOG_E(TAG, "Tâche beacon bloquée");
    return false;
  }
  if (isTaskStale(HB_SENSORS, settings.energy.poll_interval_ms + 15000)) {
    LOG_E(TAG, "Tâche sensors bloquée");
    return false;
  }
  if (isTaskStale(HB_ENERGY, settings.energy.poll_interval_ms + 15000)) {
    LOG_E(TAG, "Tâche energy bloquée");
    return false;
  }
  if (isTaskStale(HB_WEATHER, settings.weather.wh65b_interval_ms + settings.weather.wh65b_rx_timeout_ms + 30000)) {
    LOG_E(TAG, "Tâche weather bloquée");
    return false;
  }
  if (isTaskStale(HB_CLI, 10000)) {
    LOG_E(TAG, "Tâche CLI bloquée");
    return false;
  }
  return true;
}

void taskWatchdog(void* params) {
  (void)params;

  if (!settings.system.watchdog_enabled) {
    LOG_I(TAG, "Watchdog désactivé (settings.system.watchdog)");
    vTaskDelete(nullptr);
    return;
  }

  uint32_t reboot_count = watchdog_hw->scratch[WDT_SCRATCH_REBOOT_COUNT];
  if (rp2040.getResetReason() == RP2040::WDT_RESET) {
    reboot_count++;
    LOG_E(TAG, "Reboot cause par watchdog (#%lu)", (unsigned long)reboot_count);
    event_log.log(EVENT_WATCHDOG_REBOOT, (int32_t)reboot_count);
  } else {
    reboot_count = 0; // reset "propre" (alimentation, reboot manuel...) : on repart à zéro
  }
  watchdog_hw->scratch[WDT_SCRATCH_REBOOT_COUNT] = reboot_count;

  if (reboot_count > WATCHDOG_MAX_CONSECUTIVE_REBOOTS) {
    LOG_E(TAG, "Trop de reboots watchdog consécutifs (%lu) : watchdog désarmé pour ce cycle, "
               "l'appareil reste up en mode dégradé plutôt que de boucler indéfiniment",
               (unsigned long)reboot_count);
    event_log.log(EVENT_WATCHDOG_TOO_MANY_REBOOTS, (int32_t)reboot_count);
    vTaskDelete(nullptr);
    return;
  }

  rp2040.wdt_begin(WATCHDOG_TIMEOUT_MS);
  LOG_I(TAG, "Watchdog matériel armé (%d ms)", WATCHDOG_TIMEOUT_MS);

  Timer healthy_uptime_timer(WATCHDOG_HEALTHY_UPTIME_MS);
  bool healthy_uptime_reached = false;

  for (;;) {
    if (allTasksAlive()) {
      rp2040.wdt_reset();

      if (!healthy_uptime_reached && healthy_uptime_timer.hasExpired()) {
        healthy_uptime_reached = true;
        watchdog_hw->scratch[WDT_SCRATCH_REBOOT_COUNT] = 0;
        LOG_D(TAG, "Uptime sain atteint, compteur de reboots watchdog remis à zéro");
      }
    } else {
      LOG_E(TAG, "Watchdog non nourri — reboot correcteur imminent (<=%dms)", WATCHDOG_TIMEOUT_MS);
    }

    vTaskDelay(pdMS_TO_TICKS(WATCHDOG_FEED_PERIOD_MS));
  }
}

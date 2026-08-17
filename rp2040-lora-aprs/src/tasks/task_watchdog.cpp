#include "tasks.h"
#include "target.h"
#include "core/Log.h"
#include "core/Boot.h"
#include "core/ScratchRegisters.h"
#include "config/Settings.h"
#include "hal/eeprom/EventLogHistory.h"
#include "task_heartbeat.h"
#include <hardware/structs/watchdog.h>
#include <Timer.h>

// Le compteur de reboots watchdog consécutifs vit dans un registre "scratch"
// du périphérique watchdog (survit à un reset matériel, contrairement à la
// RAM normale) : au-delà de WATCHDOG_MAX_CONSECUTIVE_REBOOTS, le chien n'est
// plus réarmé pour ce cycle — mieux vaut un appareil up en mode dégradé
// qu'une boucle de reboot infinie sur un site difficile d'accès.

extern Settings settings;
extern EventLogHistory event_log;

#define TAG "WDT"
#define WATCHDOG_TIMEOUT_MS         8000              // proche du max matériel RP2040 (~8.3s)
#define WATCHDOG_FEED_PERIOD_MS     2000
#define WATCHDOG_HEALTHY_UPTIME_MS  (5UL * 60UL * 1000UL)  // 5 min sans blocage = boot considéré sain
#define WATCHDOG_MAX_CONSECUTIVE_REBOOTS 5

#define WDT_SCRATCH_REBOOT_COUNT SCRATCH_WDT_REBOOT_COUNT

static bool isTaskStale(HeartbeatTask task, uint32_t max_age_ms) {
  unsigned long last = g_heartbeat_ms[task];
  if (last == 0) {
    return false; // pas encore démarrée / pas créée dans ce mode : jamais "figée"
  }
  return (millis() - last) > max_age_ms;
}

// HB_MESH/HB_APRS ne se rafraîchissent qu'au réveil radio réel (jusqu'à
// *_TASK_MAX_WAIT_MS = 60s), donc leur marge doit rester bien au-dessus pour
// ne pas confondre un silence radio normal avec une tâche bloquée.
#define HB_RADIO_STALE_MS 90000
static bool allTasksAlive() {
  if (isTaskStale(HB_MESH, HB_RADIO_STALE_MS)) {
    LOG_E(TAG, "Tâche mesh bloquée");
    return false;
  }
  if (isTaskStale(HB_APRS, HB_RADIO_STALE_MS)) {
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

  bool wdt_armed = settings.system.watchdog_enabled;

  if (!wdt_armed) {
    LOG_I(TAG, "Watchdog désactivé (settings.system.watchdog)");
  } else {
    uint32_t reboot_count = watchdog_hw->scratch[WDT_SCRATCH_REBOOT_COUNT];
    if (rp2040.getResetReason() == RP2040::WDT_RESET) {
      reboot_count++;
      LOG_E(TAG, "Reboot cause par watchdog (#%lu)", (unsigned long)reboot_count);
      event_log.log(EVENT_WATCHDOG_REBOOT, (int32_t)reboot_count);
    } else {
      reboot_count = 0;
    }
    watchdog_hw->scratch[WDT_SCRATCH_REBOOT_COUNT] = reboot_count;

    if (reboot_count > WATCHDOG_MAX_CONSECUTIVE_REBOOTS) {
      LOG_E(TAG, "Trop de reboots watchdog consécutifs (%lu) : watchdog désarmé pour ce cycle, "
                 "l'appareil reste up en mode dégradé plutôt que de boucler indéfiniment",
                 (unsigned long)reboot_count);
      event_log.log(EVENT_WATCHDOG_TOO_MANY_REBOOTS, (int32_t)reboot_count);
      wdt_armed = false;
    } else {
      rp2040.wdt_begin(WATCHDOG_TIMEOUT_MS);
      LOG_I(TAG, "Watchdog matériel armé (%d ms)", WATCHDOG_TIMEOUT_MS);
    }
  }

  Timer healthy_uptime_timer(WATCHDOG_HEALTHY_UPTIME_MS);
  bool healthy_uptime_reached = false;

  for (;;) {
    // Retenté ici tant que la puce RTC externe n'est pas détectée (glitch I2C
    // au boot, etc.) ; s'arrête de lui-même une fois isPresent() vrai.
    if (!externalRtc.isPresent()) {
      bootInitRtc();
    }

    if (!wdt_armed) {
      // rien à nourrir, mais la boucle continue (retry RTC ci-dessus)
    } else if (allTasksAlive()) {
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

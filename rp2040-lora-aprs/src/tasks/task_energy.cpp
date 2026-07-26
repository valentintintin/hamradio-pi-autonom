// ============================================================================
// Task énergie — orchestration : watchdog MPPT + coupure/reprise basse-
// tension + réveil périodique par relais + alerte extinction MPPT.
//
// La logique de chaque volet vit dans src/energy/ (LowVoltageCutoffController,
// RelayPeriodicController, MpptShutdownMonitor) ; cette tâche se contente de
// les construire (cf. main.cpp) et de les faire tourner à chaque tick, comme
// task_beacon.cpp le fait pour AprsEngine.
//
// Ne lit plus les capteurs elle-même (cf. task_sensors.cpp, seul écrivain de
// `telemetry`) : cette tâche ne fait que réagir aux valeurs déjà publiées.
// ============================================================================

#include "tasks.h"
#include "core/Log.h"
#include "config/Settings.h"
#include "hal/Telemetry.h"
#include "hal/Ina3221Hal.h"
#include "hal/MpptChargerHal.h"
#include "hal/ChargeControllerHal.h"
#include "energy/LowVoltageCutoffController.h"
#include "energy/RelayPeriodicController.h"
#include "energy/MpptShutdownMonitor.h"
#include "task_heartbeat.h"
#include <Timer.h>

extern TelemetryData telemetry;
extern Settings settings;
extern Ina3221Hal ina3221;
extern MpptChargerHal mppt;
extern ChargeControllerHal* active_charger; // MPPT ou Victron, un seul à la fois (cf. main.cpp)
extern LowVoltageCutoffController low_voltage_cutoff;
extern RelayPeriodicController relay_periodic;
extern MpptShutdownMonitor mppt_shutdown_monitor;

#define TAG "ENERGY"
#define ENERGY_BOOT_DELAY_MS (10 * 1000)

// Tension batterie à surveiller : le mini des sources disponibles (chargeur
// solaire actif et/ou INA3221), pour couper si l'une des deux indique une
// tension basse — plus prudent que de dépendre d'une seule source. Une
// source non initialisée est ignorée plutôt que de faire chuter le mini à 0
// (ce qui déclencherait la coupure en permanence). Retourne false si aucune
// source n'est disponible.
static bool getBatteryVoltageMv(float& voltage_mv) {
  bool have_reading = false;

  if (active_charger) {
    voltage_mv = telemetry.battery_mppt.voltage_mv;
    have_reading = true;
  }
  if (ina3221.isInitialized()) {
    float v = telemetry.battery_ina.voltage_mv;
    voltage_mv = have_reading ? min(voltage_mv, v) : v;
    have_reading = true;
  }

  return have_reading;
}

void taskEnergy(void* params) {
  (void)params;
  vTaskDelay(pdMS_TO_TICKS(ENERGY_BOOT_DELAY_MS));
  LOG_D(TAG, "Task démarrée");

  mppt_shutdown_monitor.begin();

  // Pousser les seuils de coupure/reprise matériels au chip s'ils sont
  // configurés (0 = laisser le réglage usine, cf. Settings.h)
  if (mppt.isInitialized()) {
    if (settings.energy.mppt_pwr_off_mv > 0) {
      mppt.setPowerOffThreshold(settings.energy.mppt_pwr_off_mv);
    }
    if (settings.energy.mppt_pwr_on_mv > 0) {
      mppt.setPowerOnThreshold(settings.energy.mppt_pwr_on_mv);
    }
  }

  Timer wdt_feed_timer(settings.energy.mppt_wdt_interval_ms);

  for (;;) {
    heartbeat(HB_ENERGY);

    if (mppt.isInitialized() && settings.energy.mppt_wdt_enabled) {
      // Resynchroniser l'intervalle si modifié à chaud
      wdt_feed_timer.setInterval(settings.energy.mppt_wdt_interval_ms, false);
      if (wdt_feed_timer.hasExpired()) {
        mppt.feedWatchdog(120);
        wdt_feed_timer.restart();
        LOG_T(TAG, "MPPT watchdog nourri");
      }
    }

    float voltage_mv = 0;
    bool have_voltage = getBatteryVoltageMv(voltage_mv);

    low_voltage_cutoff.update(voltage_mv, have_voltage);
    relay_periodic.update(voltage_mv, have_voltage);
    mppt_shutdown_monitor.update();

    vTaskDelay(pdMS_TO_TICKS(settings.energy.poll_interval_ms));
  }
}

#include "tasks.h"
#include "core/Log.h"
#include "config/Settings.h"
#include "../hal/relay/Relay.h"
#include "hal/Telemetry.h"
#include "hal/sensors/Ina3221Hal.h"
#include "hal/chargers/MpptChargerHal.h"
#include "hal/chargers/ChargeControllerHal.h"
#include "hal/relay/RelayHal.h"
#include "energy/MpptShutdownMonitor.h"
#include "task_heartbeat.h"
#include <Timer.h>

extern TelemetryData telemetry;
extern Settings settings;
extern Ina3221Hal ina3221;
extern MpptChargerHal mppt;
extern ChargeControllerHal* active_charger;
extern MpptShutdownMonitor mppt_shutdown_monitor;
extern Relay relays[RELAY_COUNT];

#define TAG "ENERGY"
#define ENERGY_BOOT_DELAY_MS (10 * 1000)

// Prend le mini des sources dispo (plus prudent qu'une seule) ; une source
// non initialisée est ignorée plutôt que de faire chuter le mini à 0, ce qui
// couperait en permanence.
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

  // 0 = laisser le réglage usine
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
      wdt_feed_timer.setInterval(settings.energy.mppt_wdt_interval_ms, false);
      if (wdt_feed_timer.hasExpired()) {
        mppt.feedWatchdog(120);
        wdt_feed_timer.restart();
        LOG_T(TAG, "MPPT watchdog nourri");
      }
    }

    float voltage_mv = 0;
    getBatteryVoltageMv(voltage_mv);

    for (auto& relay : relays) {
      relay.update(voltage_mv);
    }

    mppt_shutdown_monitor.update();

    vTaskDelay(pdMS_TO_TICKS(settings.energy.poll_interval_ms));
  }
}

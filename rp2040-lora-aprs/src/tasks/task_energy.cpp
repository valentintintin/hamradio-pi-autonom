// ============================================================================
// Task énergie — polling périodique INA3221 + MPPT + BME280 + Victron
//                + enregistrement historique EEPROM
// ============================================================================

#include "tasks.h"
#include "config/Log.h"
#include "config/Settings.h"
#include "hal/Telemetry.h"
#include "hal/TelemetryHistory.h"
#include "hal/I2CBus.h"
#include "hal/Ina3221Hal.h"
#include "hal/MpptChargerHal.h"
#include "hal/Bme280Hal.h"
#include "hal/VictronHal.h"
#include <Timer.h>

extern TelemetryData telemetry;
extern Settings settings;
extern I2CBus i2c_bus;
extern Ina3221Hal ina3221;
extern MpptChargerHal mppt;
extern Bme280Hal bme280;
extern VictronHal victron;
extern TelemetryHistory telemetry_history;

#define TAG "ENERGY"
#define ENERGY_BOOT_DELAY_MS (10 * 1000)

void taskEnergy(void* params) {
  (void)params;
  vTaskDelay(pdMS_TO_TICKS(ENERGY_BOOT_DELAY_MS));
  LOG_D(TAG, "Task démarrée");

  Timer wdt_feed_timer(settings.energy.mppt_wdt_interval_ms);
  Timer history_timer(settings.system.telemetry_log_interval_ms);

  for (;;) {
    if (ina3221.isInitialized() && !ina3221.query(telemetry)) {
      LOG_W(TAG, "Erreur lecture INA3221");
    }

    if (mppt.isInitialized()) {
      if (!mppt.query(telemetry)) {
        LOG_W(TAG, "Erreur lecture MPPT");
      }

      if (settings.energy.mppt_wdt_enabled) {
        // Resynchroniser l'intervalle si modifié à chaud
        wdt_feed_timer.setInterval(settings.energy.mppt_wdt_interval_ms, false);
        if (wdt_feed_timer.hasExpired()) {
          mppt.feedWatchdog(120);
          wdt_feed_timer.restart();
          LOG_T(TAG, "MPPT watchdog nourri");
        }
      }
    }

    if (bme280.isInitialized() && !bme280.query(telemetry.weather)) {
      LOG_W(TAG, "Erreur lecture BME280");
    }

    if (settings.energy.victron_enabled && victron.isInitialized() && !victron.query(telemetry)) {
      LOG_W(TAG, "Erreur lecture Victron");
    }

    telemetry.uptime_s = millis() / 1000;
    telemetry.last_update_ms = millis();

    // Enregistrement EEPROM périodique
    history_timer.setInterval(settings.system.telemetry_log_interval_ms, false);
    if (telemetry_history.isInitialized() && history_timer.hasExpired()) {
      telemetry_history.record(telemetry);
      history_timer.restart();
      LOG_T(TAG, "Historique EEPROM: %d/%d",
        telemetry_history.getCount(), telemetry_history.getMaxRecords());
    }

    LOG_T(TAG, "Bat:%.0fmV/%.0fmA Sol:%.0fmV/%.0fmA",
      telemetry.battery.voltage_mv, telemetry.battery.current_ma,
      telemetry.solar.voltage_mv, telemetry.solar.current_ma);

    vTaskDelay(pdMS_TO_TICKS(settings.energy.poll_interval_ms));
  }
}

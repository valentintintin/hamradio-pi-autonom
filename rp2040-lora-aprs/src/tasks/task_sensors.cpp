// ============================================================================
// Task capteurs — polling périodique INA3221 + MPPT + BME280 + Victron
//                 + enregistrement historique EEPROM
// ============================================================================

#include "tasks.h"
#include "core/Log.h"
#include "config/Settings.h"
#include "hal/Telemetry.h"
#include "hal/eeprom/TelemetryHistory.h"
#include "hal/sensors/Ina3221Hal.h"
#include "hal/sensors/Bme280Hal.h"
#include "hal/chargers/ChargeControllerHal.h"
#include "task_heartbeat.h"
#include <Timer.h>

extern TelemetryData telemetry;
extern Settings settings;
extern Ina3221Hal ina3221;
extern Bme280Hal bme280;
extern TelemetryHistory telemetry_history;
extern ChargeControllerHal* active_charger; // MPPT ou Victron, un seul à la fois (cf. main.cpp)

#define TAG "SENSORS"
#define SENSORS_BOOT_DELAY_MS (10 * 1000)

void taskSensors(void* params) {
  (void)params;
  vTaskDelay(pdMS_TO_TICKS(SENSORS_BOOT_DELAY_MS));
  LOG_D(TAG, "Task démarrée");

  Timer history_timer(settings.system.telemetry_log_interval_ms);

  for (;;) {
    heartbeat(HB_SENSORS);

    if (ina3221.isInitialized() && !ina3221.query(telemetry)) {
      LOG_W(TAG, "Erreur lecture INA3221");
    }

    // Un seul chargeur solaire à la fois (MPPT ou Victron, cf. main.cpp).
    if (active_charger && !active_charger->query(telemetry)) {
      LOG_W(TAG, "Erreur lecture chargeur solaire");
    }

    if (bme280.isInitialized() && !bme280.query(telemetry.weather_inside)) {
      LOG_W(TAG, "Erreur lecture BME280");
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

    LOG_T(TAG, "INA Bat:%.0fmV/%.0fmA Sol:%.0fmV/%.0fmA",
      telemetry.battery_ina.voltage_mv, telemetry.battery_ina.current_ma,
      telemetry.solar_ina.voltage_mv, telemetry.solar_ina.current_ma);

    LOG_T(TAG, "MPPT Bat:%.0fmV/%.0fmA Sol:%.0fmV/%.0fmA",
      telemetry.battery_mppt.voltage_mv, telemetry.battery_mppt.current_ma,
      telemetry.solar_mppt.voltage_mv, telemetry.solar_mppt.current_ma);

    vTaskDelay(pdMS_TO_TICKS(settings.energy.poll_interval_ms));
  }
}

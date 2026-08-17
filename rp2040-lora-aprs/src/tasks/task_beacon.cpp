#include "tasks.h"
#include "core/Log.h"
#include "config/Settings.h"
#include "../aprs/AprsEngine.h"
#include "task_heartbeat.h"
#include <Timer.h>

extern AprsEngine aprs_engine;
extern Settings settings;

#define TAG "BEACON"

#define BEACON_BOOT_DELAY   (90 * 1000)   // délai post-boot anti TX storm
#define BEACON_CHECK_PERIOD (10 * 1000)

void taskAprsBeacon(void* params) {
  (void)params;

  vTaskDelay(pdMS_TO_TICKS(BEACON_BOOT_DELAY));
  LOG_D(TAG, "Task démarrée");

  aprs_engine.sendPosition(settings.aprs.comment);
  vTaskDelay(pdMS_TO_TICKS(5000));
  aprs_engine.sendTelemetryParams();
  vTaskDelay(pdMS_TO_TICKS(3000));
  aprs_engine.sendWeather();

  Timer position_timer(settings.aprs.intervalPosition_ms);
  Timer telemetry_timer(settings.aprs.intervalTelemetry_ms);
  Timer weather_timer(settings.aprs.intervalWeather_ms);

  for (;;) {
    heartbeat(HB_BEACON);

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

    if (aprs_engine.sendStatusIfChanged(settings.aprs.intervalStatus_ms)) {
      LOG_T(TAG, "TX statut");
    }

    vTaskDelay(pdMS_TO_TICKS(BEACON_CHECK_PERIOD));
  }
}

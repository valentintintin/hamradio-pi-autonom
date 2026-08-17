#include "tasks.h"
#include "target.h"
#include "core/Log.h"
#include "config/Settings.h"
#include "aprs/AprsDispatcher.h"
#include "hal/Telemetry.h"
#include "task_heartbeat.h"
#include <FineOffsetWH65B.h>
#include <string.h>

extern AprsDispatcher aprs_dispatcher;
extern TelemetryData telemetry;
extern Settings settings;

#define TAG "WEATHER"

#define WEATHER_BOOT_DELAY_MS (120 * 1000)

// raw_out (optionnel) reçoit une copie des octets bruts pour le relais FSK ;
// non rempli si retour false avant lecture d'un paquet.
static bool listenAndDecode(uint8_t* raw_out = nullptr) {
  uint8_t buffer[WH65B_PAYLOAD_LEN] = {0};
  float rssi = 0;

  if (!aprs_radio_hw.receiveWh65bFrame(settings.weather.wh65b_rx_timeout_ms, buffer, &rssi)) {
    LOG_W(TAG, "Timeout %lums, aucun paquet WH65B", settings.weather.wh65b_rx_timeout_ms);
    return false;
  }

  if (raw_out) {
    memcpy(raw_out, buffer, WH65B_PAYLOAD_LEN);
  }

  WH65BData data = FineOffsetWH65B::decode(buffer);

  if (!data.crc_ok) {
    LOG_W(TAG, "CRC invalide");
    return false;
  }
  if (data.temperature_C >= 70 || data.temperature_C <= -30) {
    LOG_W(TAG, "Température hors limites: %.1f", data.temperature_C);
    return false;
  }

  telemetry.weather_outside.is_valid = true;
  telemetry.weather_outside.wind_avg_ms = data.wind_avg_m_s;
  telemetry.weather_outside.wind_max_ms = data.wind_max_m_s;
  telemetry.weather_outside.wind_dir_deg = data.wind_dir_deg;
  telemetry.weather_outside.rain_mm = data.rainfall_mm;
  telemetry.weather_outside.light_lux = data.light_lux;
  telemetry.weather_outside.uv_index = data.uvi;

  LOG_I(TAG, "WH65B T:%.1fC H:%d%% V:%.1fm/s D:%d R:%.1fmm UV:%d RSSI:%.0f",
    data.temperature_C, data.humidity, data.wind_avg_m_s,
    data.wind_dir_deg, data.rainfall_mm, data.uvi, rssi);

  return true;
}

void taskWeather(void* params) {
  (void)params;
  vTaskDelay(pdMS_TO_TICKS(WEATHER_BOOT_DELAY_MS));
  LOG_D(TAG, "Task démarrée");

  for (;;) {
    heartbeat(HB_WEATHER);

    if (settings.weather.wh65b_enabled) {
      LOG_T(TAG, "Début cycle FSK");

      aprs_dispatcher.pause();
      bool ok = aprs_radio_hw.switchToFsk();
      if (ok) {
        uint8_t raw[WH65B_PAYLOAD_LEN];
        if (listenAndDecode(raw) && settings.weather.resend_enabled) {
          // Relais RF protocole brut (pas une conversion APRS) : le délai
          // laisse le temps à la station d'origine de finir son propre TX.
          vTaskDelay(pdMS_TO_TICKS(settings.weather.resend_delay_ms));
          if (aprs_radio_hw.relayWh65bFrame(raw, WH65B_PAYLOAD_LEN, settings.weather.resend_power_dbm)) {
            LOG_D(TAG, "Trame WH65B relayée (%d dBm)", settings.weather.resend_power_dbm);
          } else {
            LOG_E(TAG, "Relais FSK échoué");
          }
        }
      }
      // Repart des settings radio.aprs.* : restaure automatiquement la
      // puissance normale après le resend_power_dbm ci-dessus.
      aprs_radio_hw.switchToLora();
      aprs_dispatcher.resume();

      LOG_T(TAG, "Fin cycle, retour APRS");
    }

    vTaskDelay(pdMS_TO_TICKS(settings.weather.wh65b_interval_ms));
  }
}

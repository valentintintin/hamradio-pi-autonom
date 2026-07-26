// ============================================================================
// Task météo — switch FSK SX1262 pour réception WH65B
//
// Périodiquement (settings.weather.wh65b_interval) :
//   1. Pause le dispatcher APRS
//   2. SX1262 433 → FSK (433.92MHz, 8.21kbps) — cf aprs/AprsRadioMode.h
//   3. Écoute max settings.weather.wh65b_timeout → decode WH65B
//   4. Restaure LoRa APRS → resume
// ============================================================================

#include "tasks.h"
#include "target.h"
#include "core/Log.h"
#include "config/Settings.h"
#include "aprs/AprsDispatcher.h"
#include "aprs/LoRa433RadioMode.h"
#include "hal/Telemetry.h"
#include "task_heartbeat.h"
#include <FineOffsetWH65B.h>
#include <RadioLib.h>
#include <string.h>

extern AprsDispatcher aprs_dispatcher;
extern TelemetryData telemetry;
extern Settings settings;

#define TAG "WEATHER"

#define WEATHER_BOOT_DELAY_MS (120 * 1000)

static volatile bool fsk_rx_flag = false;
static void onFskRxDone() {
  fsk_rx_flag = true;
}

// ============================================================================
// Écoute FSK + décodage
//
// raw_out (optionnel) reçoit une copie des WH65B_PAYLOAD_LEN octets bruts
// reçus, pour le relais FSK (cf. taskWeather() ci-dessous) — non rempli si la
// fonction retourne false avant d'avoir lu un paquet.
// ============================================================================
static bool listenAndDecode(uint8_t* raw_out = nullptr) {
  fsk_rx_flag = false;

  int16_t state = aprs_radio_hw.startReceive();
  if (state != RADIOLIB_ERR_NONE) {
    LOG_E(TAG, "startReceive FSK: %d", state);
    return false;
  }

  unsigned long start = millis();
  while (!fsk_rx_flag) {
    if (millis() - start > settings.weather.wh65b_rx_timeout_ms) {
      LOG_W(TAG, "Timeout %lums, aucun paquet WH65B", settings.weather.wh65b_rx_timeout_ms);
      return false;
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }

  uint8_t buffer[WH65B_PAYLOAD_LEN];
  state = aprs_radio_hw.readData(buffer, WH65B_PAYLOAD_LEN);
  if (state != RADIOLIB_ERR_NONE) {
    LOG_E(TAG, "readData: %d", state);
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

  // Stocker
  telemetry.weather_outside.is_valid = true;
  telemetry.weather_outside.wind_avg_ms = data.wind_avg_m_s;
  telemetry.weather_outside.wind_max_ms = data.wind_max_m_s;
  telemetry.weather_outside.wind_dir_deg = data.wind_dir_deg;
  telemetry.weather_outside.rain_mm = data.rainfall_mm;
  telemetry.weather_outside.light_lux = data.light_lux;
  telemetry.weather_outside.uv_index = data.uvi;

  float rssi = aprs_radio_hw.getRSSI();
  LOG_I(TAG, "WH65B T:%.1fC H:%d%% V:%.1fm/s D:%d R:%.1fmm UV:%d RSSI:%.0f",
    data.temperature_C, data.humidity, data.wind_avg_m_s,
    data.wind_dir_deg, data.rainfall_mm, data.uvi, rssi);

  return true;
}

// ============================================================================
// Task FreeRTOS
// ============================================================================
void taskWeather(void* params) {
  (void)params;
  vTaskDelay(pdMS_TO_TICKS(WEATHER_BOOT_DELAY_MS));
  LOG_D(TAG, "Task démarrée");

  for (;;) {
    heartbeat(HB_WEATHER);

    if (settings.weather.wh65b_enabled) {
      LOG_T(TAG, "Début cycle FSK");

      aprs_dispatcher.pause();
      bool ok = LoRa433RadioMode::switchToFsk(onFskRxDone);
      if (ok) {
        uint8_t raw[WH65B_PAYLOAD_LEN];
        if (listenAndDecode(raw) && settings.weather.resend_enabled) {
          // Relais RF protocole (pas une conversion APRS) : d'autres stations
          // WH65B à portée (dont celle de l'utilisateur, ~1km) écoutent
          // directement ce format — on retransmet les octets bruts tels
          // quels, même fréquence, après un délai (laisse le temps à la
          // station d'origine de terminer son propre cycle TX).
          vTaskDelay(pdMS_TO_TICKS(settings.weather.resend_delay_ms));
          aprs_radio_hw.setOutputPower(settings.weather.resend_power_dbm);
          int16_t state = aprs_radio_hw.transmit(raw, WH65B_PAYLOAD_LEN);
          if (state != RADIOLIB_ERR_NONE) {
            LOG_E(TAG, "Relais FSK: %d", state);
          } else {
            LOG_D(TAG, "Trame WH65B relayée (%d dBm)", settings.weather.resend_power_dbm);
          }
        }
      }
      // switchToLora() repart des settings (radio.aprs.*), y compris la
      // puissance normale : restaure automatiquement après le relais
      // resend_power_dbm ci-dessus, pas de reapplication manuelle nécessaire.
      LoRa433RadioMode::switchToLora();
      aprs_dispatcher.resume();

      LOG_T(TAG, "Fin cycle, retour APRS");
    }

    vTaskDelay(pdMS_TO_TICKS(settings.weather.wh65b_interval_ms));
  }
}

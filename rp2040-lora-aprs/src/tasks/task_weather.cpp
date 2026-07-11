// ============================================================================
// Task météo — switch FSK SX1262 pour réception WH65B
//
// Toutes les 10 minutes :
//   1. Pause le dispatcher APRS
//   2. SX1262 433 → FSK (433.92MHz, 8.21kbps)
//   3. Écoute max 60s → decode WH65B
//   4. Restaure LoRa APRS → resume
// ============================================================================

#include "tasks.h"
#include "target.h"
#include "config/Log.h"
#include "aprs/AprsDispatcher.h"
#include "hal/Telemetry.h"
#include <FineOffsetWH65B.h>
#include <RadioLib.h>

extern AprsDispatcher aprs_dispatcher;
extern TelemetryData telemetry;

#define TAG "WEATHER"

// --- Constantes WH65B FSK --------------------------------------------------
#define WH65B_FREQ           433.92f
#define WH65B_BITRATE        8.21f
#define WH65B_FREQ_DEV       57.136f
#define WH65B_RX_BW          270.0f
#define WH65B_PAYLOAD_LEN    27
#define WH65B_SYNC_WORD_0    0xAA
#define WH65B_SYNC_WORD_1    0x2D

#define WEATHER_INTERVAL_MS  (10 * 60 * 1000)
#define WEATHER_RX_TIMEOUT_MS (60 * 1000)
#define WEATHER_BOOT_DELAY_MS (120 * 1000)

static volatile bool fsk_rx_flag = false;
static void onFskRxDone() { fsk_rx_flag = true; }

// ============================================================================
// Switch SX1262 → FSK
// ============================================================================
static bool switchToFsk() {
  int16_t state = aprs_radio_hw.beginFSK(
    WH65B_FREQ, WH65B_BITRATE, WH65B_FREQ_DEV, WH65B_RX_BW, APRS_TX_POWER, 8);

  if (state != RADIOLIB_ERR_NONE) { LOG_E(TAG, "beginFSK: %d", state); return false; }

  state = aprs_radio_hw.setEncoding(RADIOLIB_ENCODING_NRZ);
  if (state != RADIOLIB_ERR_NONE) { LOG_E(TAG, "setEncoding: %d", state); return false; }

  state = aprs_radio_hw.setCRC(0);
  if (state != RADIOLIB_ERR_NONE) { LOG_E(TAG, "setCRC: %d", state); return false; }

  state = aprs_radio_hw.fixedPacketLengthMode(WH65B_PAYLOAD_LEN);
  if (state != RADIOLIB_ERR_NONE) { LOG_E(TAG, "fixedPacketLen: %d", state); return false; }

  uint8_t sync[] = { WH65B_SYNC_WORD_0, WH65B_SYNC_WORD_1 };
  state = aprs_radio_hw.setSyncWord(sync, 2);
  if (state != RADIOLIB_ERR_NONE) { LOG_E(TAG, "setSyncWord: %d", state); return false; }

  aprs_radio_hw.setDio2AsRfSwitch(APRS_DIO2_AS_RF_SWITCH);
  aprs_radio_hw.setRfSwitchPins(APRS_RXEN, RADIOLIB_NC);
  aprs_radio_hw.setPacketReceivedAction(onFskRxDone);

  LOG_D(TAG, "FSK configuré, écoute...");
  return true;
}

// ============================================================================
// Restaure SX1262 → LoRa APRS
// ============================================================================
static bool switchBackToLora() {
  aprs_radio_hw.clearPacketReceivedAction();

  int16_t state = aprs_radio_hw.begin(
    APRS_FREQ, APRS_BW, APRS_SF, APRS_CR,
    RADIOLIB_SX126X_SYNC_WORD_PRIVATE, APRS_TX_POWER, 8);

  if (state != RADIOLIB_ERR_NONE) {
    state = aprs_radio_hw.begin(
      APRS_FREQ, APRS_BW, APRS_SF, APRS_CR,
      RADIOLIB_SX126X_SYNC_WORD_PRIVATE, APRS_TX_POWER, 8, 0);
  }

  if (state != RADIOLIB_ERR_NONE) {
    LOG_E(TAG, "Retour LoRa FAIL: %d", state);
    return false;
  }

  aprs_radio_hw.setCRC(2);
  aprs_radio_hw.setCurrentLimit(APRS_CURRENT_LIMIT);
  aprs_radio_hw.setDio2AsRfSwitch(APRS_DIO2_AS_RF_SWITCH);
  aprs_radio_hw.setRxBoostedGainMode(APRS_RX_BOOSTED_GAIN);
  aprs_radio_hw.setRfSwitchPins(APRS_RXEN, RADIOLIB_NC);

  LOG_D(TAG, "Retour LoRa OK");
  return true;
}

// ============================================================================
// Écoute FSK + décodage
// ============================================================================
static bool listenAndDecode() {
  fsk_rx_flag = false;

  int16_t state = aprs_radio_hw.startReceive();
  if (state != RADIOLIB_ERR_NONE) {
    LOG_E(TAG, "startReceive FSK: %d", state);
    return false;
  }

  unsigned long start = millis();
  while (!fsk_rx_flag) {
    if (millis() - start > WEATHER_RX_TIMEOUT_MS) {
      LOG_W(TAG, "Timeout %ds, aucun paquet WH65B", WEATHER_RX_TIMEOUT_MS / 1000);
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
  telemetry.weather.wh65b_valid = true;
  telemetry.weather.wind_avg_ms = data.wind_avg_m_s;
  telemetry.weather.wind_max_ms = data.wind_max_m_s;
  telemetry.weather.wind_dir_deg = data.wind_dir_deg;
  telemetry.weather.rain_mm = data.rainfall_mm;
  telemetry.weather.light_lux = data.light_lux;
  telemetry.weather.uv_index = data.uvi;

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
    LOG_T(TAG, "Début cycle FSK");

    aprs_dispatcher.pause();
    bool ok = switchToFsk();
    if (ok) listenAndDecode();
    switchBackToLora();
    aprs_dispatcher.resume();

    LOG_T(TAG, "Fin cycle, retour APRS");
    vTaskDelay(pdMS_TO_TICKS(WEATHER_INTERVAL_MS));
  }
}

#include "AprsRadioHwReal.h"
#include "config/Settings.h"
#include "core/Log.h"
#include <Arduino.h>
#include <RadioLib.h>
#include <FineOffsetWH65B.h>  // WH65B_PAYLOAD_LEN
#include <task.h>

extern Settings settings;

#define TAG "APRS-RADIO"

#define WH65B_FREQ        433.92f
#define WH65B_BITRATE     8.21f
#define WH65B_FREQ_DEV    57.136f
#define WH65B_RX_BW       270.0f
#define WH65B_SYNC_WORD_0 0xAA
#define WH65B_SYNC_WORD_1 0x2D

// RadioLib exige un pointeur de fonction C (pas de méthode/lambda capturante)
// pour setPacketReceivedAction() — un seul SX1262 APRS existe jamais qu'en un
// seul exemplaire (cf. variant/target.cpp), donc un handle fichier-statique
// est aussi correct qu'un membre. Réveille directement la tâche météo
// (task_weather.cpp) bloquée dans receiveWh65bFrame() ci-dessous, au lieu de
// poser un flag relu par polling.
static TaskHandle_t fsk_waiting_task = nullptr;
static void onFskRxDone() {
  if (fsk_waiting_task) {
    BaseType_t woken = pdFALSE;
    vTaskNotifyGiveFromISR(fsk_waiting_task, &woken);
    portYIELD_FROM_ISR(woken);
  }
}

bool AprsRadioHwReal::switchToFsk() {
  int16_t state = _hw.beginFSK(
    WH65B_FREQ, WH65B_BITRATE, WH65B_FREQ_DEV, WH65B_RX_BW, settings.weather.resend_power_dbm, 8);

  if (state != RADIOLIB_ERR_NONE) {
    LOG_E(TAG, "beginFSK: %d", state);
    return false;
  }

  state = _hw.setEncoding(RADIOLIB_ENCODING_NRZ);
  if (state != RADIOLIB_ERR_NONE) {
    LOG_E(TAG, "setEncoding: %d", state);
    return false;
  }

  state = _hw.setCRC(0);
  if (state != RADIOLIB_ERR_NONE) {
    LOG_E(TAG, "setCRC: %d", state);
    return false;
  }

  state = _hw.fixedPacketLengthMode(WH65B_PAYLOAD_LEN);
  if (state != RADIOLIB_ERR_NONE) {
    LOG_E(TAG, "fixedPacketLen: %d", state);
    return false;
  }

  uint8_t sync[] = { WH65B_SYNC_WORD_0, WH65B_SYNC_WORD_1 };
  state = _hw.setSyncWord(sync, 2);
  if (state != RADIOLIB_ERR_NONE) {
    LOG_E(TAG, "setSyncWord: %d", state);
    return false;
  }

  _hw.setDio2AsRfSwitch(APRS_DIO2_AS_RF_SWITCH);
  _hw.setRfSwitchPins(APRS_RXEN, RADIOLIB_NC);
  _hw.setPacketReceivedAction(onFskRxDone);

  LOG_D(TAG, "FSK configuré, écoute...");
  return true;
}

bool AprsRadioHwReal::switchToLora() {
  _hw.clearPacketReceivedAction();

  int16_t state = _hw.begin(
    settings.radio.aprs_freq, settings.radio.aprs_bw, settings.radio.aprs_sf, settings.radio.aprs_cr,
    RADIOLIB_SX126X_SYNC_WORD_PRIVATE, settings.radio.aprs_tx_power, 8);

  if (state != RADIOLIB_ERR_NONE) {
    state = _hw.begin(
      settings.radio.aprs_freq, settings.radio.aprs_bw, settings.radio.aprs_sf, settings.radio.aprs_cr,
      RADIOLIB_SX126X_SYNC_WORD_PRIVATE, settings.radio.aprs_tx_power, 8, 0);
  }

  if (state != RADIOLIB_ERR_NONE) {
    LOG_E(TAG, "Retour LoRa FAIL: %d", state);
    return false;
  }

  _hw.setCRC(2);
  _hw.setCurrentLimit(APRS_CURRENT_LIMIT);
  _hw.setDio2AsRfSwitch(APRS_DIO2_AS_RF_SWITCH);
  _hw.setRxBoostedGainMode(APRS_RX_BOOSTED_GAIN);
  _hw.setRfSwitchPins(APRS_RXEN, RADIOLIB_NC);

  LOG_D(TAG, "Retour LoRa OK");
  return true;
}

bool AprsRadioHwReal::receiveWh65bFrame(uint32_t timeoutMs, uint8_t* outBuf, float* outRssi) {
  // Renseigné avant startReceive() : l'IRQ ne peut matériellement pas se
  // déclencher avant que le module soit effectivement mis en écoute, donc pas
  // de course possible avec onFskRxDone() ci-dessus.
  fsk_waiting_task = xTaskGetCurrentTaskHandle();

  int16_t state = _hw.startReceive();
  if (state != RADIOLIB_ERR_NONE) {
    LOG_E(TAG, "startReceive FSK: %d", state);
    fsk_waiting_task = nullptr;
    return false;
  }

  uint32_t got = ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(timeoutMs));
  fsk_waiting_task = nullptr;
  if (!got) {
    return false;  // timeout, aucune trame WH65B
  }

  state = _hw.readData(outBuf, WH65B_PAYLOAD_LEN);
  if (state != RADIOLIB_ERR_NONE) {
    LOG_E(TAG, "readData: %d", state);
    return false;
  }

  if (outRssi) {
    *outRssi = _hw.getRSSI();
  }
  return true;
}

bool AprsRadioHwReal::relayWh65bFrame(const uint8_t* data, size_t len, int8_t powerDbm) {
  _hw.setOutputPower(powerDbm);
  int16_t state = _hw.transmit(data, len);
  if (state != RADIOLIB_ERR_NONE) {
    LOG_E(TAG, "Relais FSK: %d", state);
    return false;
  }
  return true;
}

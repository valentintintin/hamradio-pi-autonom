#include "LoRa433RadioMode.h"
#include "target.h"
#include "core/Log.h"
#include <RadioLib.h>

#define TAG "APRS-RADIO"

#define WH65B_FREQ        433.92f
#define WH65B_BITRATE     8.21f
#define WH65B_FREQ_DEV    57.136f
#define WH65B_RX_BW       270.0f
#define WH65B_SYNC_WORD_0 0xAA
#define WH65B_SYNC_WORD_1 0x2D

namespace LoRa433RadioMode {

bool switchToFsk(AprsRadioRxCallback onRxDone) {
  int16_t state = aprs_radio_hw.beginFSK(
    WH65B_FREQ, WH65B_BITRATE, WH65B_FREQ_DEV, WH65B_RX_BW, APRS_TX_POWER, 8);

  if (state != RADIOLIB_ERR_NONE) {
    LOG_E(TAG, "beginFSK: %d", state);
    return false;
  }

  state = aprs_radio_hw.setEncoding(RADIOLIB_ENCODING_NRZ);
  if (state != RADIOLIB_ERR_NONE) {
    LOG_E(TAG, "setEncoding: %d", state);
    return false;
  }

  state = aprs_radio_hw.setCRC(0);
  if (state != RADIOLIB_ERR_NONE) {
    LOG_E(TAG, "setCRC: %d", state);
    return false;
  }

  state = aprs_radio_hw.fixedPacketLengthMode(WH65B_PAYLOAD_LEN);
  if (state != RADIOLIB_ERR_NONE) {
    LOG_E(TAG, "fixedPacketLen: %d", state);
    return false;
  }

  uint8_t sync[] = { WH65B_SYNC_WORD_0, WH65B_SYNC_WORD_1 };
  state = aprs_radio_hw.setSyncWord(sync, 2);
  if (state != RADIOLIB_ERR_NONE) {
    LOG_E(TAG, "setSyncWord: %d", state);
    return false;
  }

  aprs_radio_hw.setDio2AsRfSwitch(APRS_DIO2_AS_RF_SWITCH);
  aprs_radio_hw.setRfSwitchPins(APRS_RXEN, RADIOLIB_NC);
  aprs_radio_hw.setPacketReceivedAction(onRxDone);

  LOG_D(TAG, "FSK configuré, écoute...");
  return true;
}

bool switchToLora() {
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

}  // namespace AprsRadioMode

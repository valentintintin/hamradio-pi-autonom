#include "target.h"

#include <Arduino.h>
#include <helpers/ArduinoHelpers.h>
#include "InternalRp2040RTCClock.h"
#include "AprsRadioHwReal.h"
#include "AprsCarrierReal.h"
#include "ExternalRtcReal.h"
#include "core/Log.h"

// ============================================================================
// Board
// ============================================================================
MyBoard board;

// ============================================================================
// LORA 868 — MeshCore (SPI1)
// ============================================================================
static RADIO_CLASS mesh_radio_hw = new Module(P_LORA_NSS, P_LORA_DIO_1, P_LORA_RESET, P_LORA_BUSY, SPI1);
WRAPPER_CLASS mesh_radio_driver(mesh_radio_hw, board);

// ============================================================================
// LORA 433 — APRS (SPI0)
// ============================================================================
CustomSX1262 aprs_radio_hw_sx1262 = new Module(P_APRS_NSS, P_APRS_DIO_1, P_APRS_RESET, P_APRS_BUSY, SPI);
CustomSX1262Wrapper aprs_radio_driver(aprs_radio_hw_sx1262, board);

// Bascule LoRa/FSK + réception/relais WH65B + séquence CW/SSTV — implémentations
// réelles (cf. AprsRadioHwReal.h/AprsCarrierReal.h), liées à `aprs_radio_hw`/
// `aprs_carrier` (déclarés dans target.h) via ces objets concrets.
static AprsRadioHwReal aprs_radio_hw_real(aprs_radio_hw_sx1262);
static AprsCarrierReal aprs_carrier_real(aprs_radio_hw_sx1262);
IAprsRadioHw& aprs_radio_hw = aprs_radio_hw_real;
IAprsCarrier& aprs_carrier = aprs_carrier_real;

// ============================================================================
// RTC + Sensors
// ============================================================================
static InternalRp2040RTCClock fallback_clock;
AutoDiscoverRTCClock rtc_clock(fallback_clock);
SensorManager sensors;

// Puce RTC externe battery-backed (RX8025T) — persistance de l'heure entre
// coupures d'alimentation (cf. hal/rtc/ExternalRtc.h). N'entre pas dans
// AutoDiscoverRTCClock (lib/MeshCore, vendorée — DS3231/RV-3028/PCF8563/
// RX8130CE seulement, pas RX8025T) : synchronisation en un point unique
// ci-dessous (mesh_radio_init(), au boot) plutôt qu'une lecture I2C à chaque
// rtc_clock.getCurrentTime().
static ExternalRtcReal external_rtc_real(Wire);
IExternalRtc& externalRtc = external_rtc_real;

// ============================================================================
// Init radio MeshCore (868 MHz, SPI1)
// ============================================================================
bool mesh_radio_init() {
  rtc_clock.begin(Wire);

  if (externalRtc.begin()) {
    uint32_t chipTime;
    if (externalRtc.readTime(&chipTime)) {
      rtc_clock.setCurrentTime(chipTime);
      LOG_I("RTC", "Horloge RP2040 synchronisée depuis la puce RX8025T");
    } else {
      LOG_W("RTC", "Puce RX8025T présente mais heure non fiable (VLF) — horloge RP2040 non synchronisée");
    }
  } else {
    LOG_W("RTC", "Puce RX8025T non détectée sur le bus I2C");
  }

  // Config SPI1
  SPI1.setSCK(P_LORA_SCLK);
  SPI1.setTX(P_LORA_MOSI);
  SPI1.setRX(P_LORA_MISO);

  pinMode(P_LORA_NSS, OUTPUT);
  digitalWrite(P_LORA_NSS, HIGH);

  SPI1.begin(false);

  // NULL = skip SPI init inside std_init (déjà fait ci-dessus)
  return mesh_radio_hw.std_init(NULL);
}

// ============================================================================
// Init radio APRS (433 MHz, SPI0)
// ============================================================================
bool aprs_radio_init() {
  // Config SPI0
  SPI.setSCK(P_APRS_SCLK);
  SPI.setTX(P_APRS_MOSI);
  SPI.setRX(P_APRS_MISO);

  pinMode(P_APRS_NSS, OUTPUT);
  digitalWrite(P_APRS_NSS, HIGH);

  SPI.begin(false);

  // Init manuelle du SX1262 pour APRS (paramètres LoRa-APRS)
  int16_t state = aprs_radio_hw_sx1262.begin(
    APRS_FREQ,       // 433.775 MHz
    APRS_BW,         // 125 kHz
    APRS_SF,         // SF12
    APRS_CR,         // CR 4/5
    RADIOLIB_SX126X_SYNC_WORD_PRIVATE,
    APRS_TX_POWER,   // 22 dBm
    8                 // preamble length
  );

  if (state != RADIOLIB_ERR_NONE) {
    // Retry sans TCXO
    state = aprs_radio_hw_sx1262.begin(
      APRS_FREQ, APRS_BW, APRS_SF, APRS_CR,
      RADIOLIB_SX126X_SYNC_WORD_PRIVATE,
      APRS_TX_POWER, 8, 0  // TCXO = 0
    );
  }

  if (state != RADIOLIB_ERR_NONE) {
    return false;
  }

  // Config supplémentaire
  aprs_radio_hw_sx1262.setCRC(2);  // CRC 2 bytes
  aprs_radio_hw_sx1262.setCurrentLimit(APRS_CURRENT_LIMIT);
  aprs_radio_hw_sx1262.setDio2AsRfSwitch(APRS_DIO2_AS_RF_SWITCH);
  aprs_radio_hw_sx1262.setRxBoostedGainMode(APRS_RX_BOOSTED_GAIN);

  // RXEN pin
  aprs_radio_hw_sx1262.setRfSwitchPins(APRS_RXEN, RADIOLIB_NC);

  return true;
}

// ============================================================================
// Identity (utilise le bruit radio pour RNG)
// ============================================================================
mesh::LocalIdentity radio_new_identity() {
  RadioNoiseListener rng(mesh_radio_hw);
  return mesh::LocalIdentity(&rng);
}

// ============================================================================
// Seed RNG depuis bruit radio
// ============================================================================
long radio_get_rng_seed() {
  long seed = 0;
  for (int i = 0; i < 4; i++) {
    seed = (seed << 8) | ((uint8_t)(mesh_radio_hw.getRSSI() * 100) & 0xFF);
    delay(10);
  }
  return seed;
}

// ============================================================================
// Reconfigurer la radio MeshCore à chaud (utilisé par CommonCLI)
// ============================================================================
void radio_set_params(float freq, float bw, uint8_t sf, uint8_t cr) {
  mesh_radio_hw.setFrequency(freq);
  mesh_radio_hw.setBandwidth(bw);
  mesh_radio_hw.setSpreadingFactor(sf);
  mesh_radio_hw.setCodingRate(cr);
}

void radio_set_tx_power(int8_t power_dbm) {
  mesh_radio_hw.setOutputPower(power_dbm);
}

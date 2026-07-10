#include "target.h"

#include <Arduino.h>
#include <helpers/ArduinoHelpers.h>

// ============================================================================
// Board
// ============================================================================
F4iseBoard board;

// ============================================================================
// LORA 868 — MeshCore (SPI1)
// ============================================================================
static RADIO_CLASS mesh_radio_hw = new Module(P_LORA_NSS, P_LORA_DIO_1, P_LORA_RESET, P_LORA_BUSY, SPI1);
WRAPPER_CLASS mesh_radio_driver(mesh_radio_hw, board);

// ============================================================================
// LORA 433 — APRS (SPI0)
// ============================================================================
CustomSX1262 aprs_radio_hw = new Module(P_APRS_NSS, P_APRS_DIO_1, P_APRS_RESET, P_APRS_BUSY, SPI);
CustomSX1262Wrapper aprs_radio_driver(aprs_radio_hw, board);

// ============================================================================
// RTC + Sensors
// ============================================================================
static VolatileRTCClock fallback_clock;
AutoDiscoverRTCClock rtc_clock(fallback_clock);
SensorManager sensors;

// ============================================================================
// Init radio MeshCore (868 MHz, SPI1)
// ============================================================================
bool mesh_radio_init() {
  rtc_clock.begin(Wire);

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
  int16_t state = aprs_radio_hw.begin(
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
    state = aprs_radio_hw.begin(
      APRS_FREQ, APRS_BW, APRS_SF, APRS_CR,
      RADIOLIB_SX126X_SYNC_WORD_PRIVATE,
      APRS_TX_POWER, 8, 0  // TCXO = 0
    );
  }

  if (state != RADIOLIB_ERR_NONE) {
    return false;
  }

  // Config supplémentaire
  aprs_radio_hw.setCRC(2);  // CRC 2 bytes
  aprs_radio_hw.setCurrentLimit(APRS_CURRENT_LIMIT);
  aprs_radio_hw.setDio2AsRfSwitch(APRS_DIO2_AS_RF_SWITCH);
  aprs_radio_hw.setRxBoostedGainMode(APRS_RX_BOOSTED_GAIN);

  // RXEN pin
  aprs_radio_hw.setRfSwitchPins(APRS_RXEN, RADIOLIB_NC);

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

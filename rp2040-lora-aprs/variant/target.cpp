#include "target.h"

#include <Arduino.h>
#include <helpers/ArduinoHelpers.h>
#include "InternalRp2040RTCClock.h"
#include "AprsRadioHwReal.h"
#include "AprsCarrierReal.h"
#include "ExternalRtcReal.h"
#include "hal/i2c/I2CBus.h"

extern I2CBus i2c_bus;

MyBoard board;

static RADIO_CLASS mesh_radio_hw = new Module(P_LORA_NSS, P_LORA_DIO_1, P_LORA_RESET, P_LORA_BUSY, SPI1);
MeshRadioWrapper mesh_radio_driver(mesh_radio_hw, board);

CustomSX1262 aprs_radio_hw_sx1262 = new Module(P_APRS_NSS, P_APRS_DIO_1, P_APRS_RESET, P_APRS_BUSY, SPI);
AprsRadioWrapper aprs_radio_driver(aprs_radio_hw_sx1262, board);

static AprsRadioHwReal aprs_radio_hw_real(aprs_radio_hw_sx1262);
static AprsCarrierReal aprs_carrier_real(aprs_radio_hw_sx1262);
IAprsRadioHw& aprs_radio_hw = aprs_radio_hw_real;
IAprsCarrier& aprs_carrier = aprs_carrier_real;

// rtc_clock est directement l'horloge interne RP2040, volontairement PAS un
// AutoDiscoverRTCClock ici : son probe RX8130CE sonde l'adresse I2C 0x32,
// qui est aussi celle du RX8025T de cette carte mais avec un protocole
// différent — il le prendrait à tort pour un RX8130CE et corromprait
// silencieusement les lectures/écritures d'heure. Le RX8025T est donc géré
// exclusivement via externalRtc, synchronisé vers rtc_clock au boot.
InternalRp2040RTCClock rtc_clock;
SensorManager sensors;

static ExternalRtcReal external_rtc_real(i2c_bus);
IExternalRtc& externalRtc = external_rtc_real;

bool mesh_radio_init() {
  SPI1.setSCK(P_LORA_SCLK);
  SPI1.setTX(P_LORA_MOSI);
  SPI1.setRX(P_LORA_MISO);

  pinMode(P_LORA_NSS, OUTPUT);
  digitalWrite(P_LORA_NSS, HIGH);

  SPI1.begin(false);

  return mesh_radio_hw.std_init(NULL); // NULL = skip l'init SPI interne à std_init, déjà faite ci-dessus
}

bool aprs_radio_init() {
  SPI.setSCK(P_APRS_SCLK);
  SPI.setTX(P_APRS_MOSI);
  SPI.setRX(P_APRS_MISO);

  pinMode(P_APRS_NSS, OUTPUT);
  digitalWrite(P_APRS_NSS, HIGH);

  SPI.begin(false);

  int16_t state = aprs_radio_hw_sx1262.begin(
    APRS_FREQ, APRS_BW, APRS_SF, APRS_CR,
    RADIOLIB_SX126X_SYNC_WORD_PRIVATE,
    APRS_TX_POWER,
    8
  );

  if (state != RADIOLIB_ERR_NONE) {
    // retry avec le dernier argument (TCXO) à 0 = désactivé
    state = aprs_radio_hw_sx1262.begin(
      APRS_FREQ, APRS_BW, APRS_SF, APRS_CR,
      RADIOLIB_SX126X_SYNC_WORD_PRIVATE,
      APRS_TX_POWER, 8, 0
    );
  }

  if (state != RADIOLIB_ERR_NONE) {
    return false;
  }

  aprs_radio_hw_sx1262.setCRC(2);
  aprs_radio_hw_sx1262.setCurrentLimit(APRS_CURRENT_LIMIT);
  aprs_radio_hw_sx1262.setDio2AsRfSwitch(APRS_DIO2_AS_RF_SWITCH);
  aprs_radio_hw_sx1262.setRxBoostedGainMode(APRS_RX_BOOSTED_GAIN);
  aprs_radio_hw_sx1262.setRfSwitchPins(APRS_RXEN, RADIOLIB_NC);

  return true;
}

mesh::LocalIdentity radio_new_identity() {
  RadioNoiseListener rng(mesh_radio_hw);
  return mesh::LocalIdentity(&rng);
}

long radio_get_rng_seed() {
  long seed = 0;
  for (int i = 0; i < 4; i++) {
    seed = (seed << 8) | ((uint8_t)(mesh_radio_hw.getRSSI() * 100) & 0xFF);
    delay(10);
  }
  return seed;
}

void radio_set_params(float freq, float bw, uint8_t sf, uint8_t cr) {
  mesh_radio_hw.setFrequency(freq);
  mesh_radio_hw.setBandwidth(bw);
  mesh_radio_hw.setSpreadingFactor(sf);
  mesh_radio_hw.setCodingRate(cr);
}

void radio_set_tx_power(int8_t power_dbm) {
  mesh_radio_hw.setOutputPower(power_dbm);
}

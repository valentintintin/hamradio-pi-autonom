#include "target.h"

#include <Arduino.h>
#include <helpers/ArduinoHelpers.h>
#include "InternalRp2040RTCClock.h"
#include "SimWorld.h"
#include "core/Log.h"
#include "AprsRadioHwSim.h"
#include "AprsCarrierSim.h"
#include "ExternalRtcSim.h"

// ============================================================================
// Board
// ============================================================================
MyBoard board;

// ============================================================================
// Radios simulées (cf. native/sim/SimRadio.h) — TX/RX passent par SimWorld
// au lieu de RadioLib+SPI réels.
// ============================================================================
SimRadio mesh_radio_driver(SimWorld::instance().mesh_log, SimWorld::instance().mesh_rx_queue, "MESH-RADIO");
SimRadio aprs_radio_driver(SimWorld::instance().aprs_log, SimWorld::instance().aprs_rx_queue, "APRS-RADIO");

// Bascule LoRa/FSK + réception/relais WH65B + séquence CW/SSTV — implémentations
// simulées (cf. AprsRadioHwSim.h/AprsCarrierSim.h).
static AprsRadioHwSim aprs_radio_hw_sim;
static AprsCarrierSim aprs_carrier_sim;
IAprsRadioHw& aprs_radio_hw = aprs_radio_hw_sim;
IAprsCarrier& aprs_carrier = aprs_carrier_sim;

static ExternalRtcSim external_rtc_sim;
IExternalRtc& externalRtc = external_rtc_sim;

// ============================================================================
// RTC + Sensors — identiques à la cible réelle : InternalRp2040RTCClock
// (variant/InternalRp2040RTCClock.h, réutilisé tel quel) retombe sur
// l'horloge murale de l'hôte (cf. native/compat/rtc_compat.cpp),
// AutoDiscoverRTCClock retombe dessus faute de RTC I2C détecté (cf.
// native/compat/Wire.h : endTransmission() échoue toujours).
// ============================================================================
static InternalRp2040RTCClock fallback_clock;
AutoDiscoverRTCClock rtc_clock(fallback_clock);
SensorManager sensors;

// ============================================================================
// Init radios — rien à initialiser matériellement, juste marquer "prêt".
// ============================================================================
bool mesh_radio_init() {
  rtc_clock.begin(Wire);
  return true;
}

bool aprs_radio_init() {
  return true;
}

// ============================================================================
// Identity / RNG — bruit radio réel remplacé par le RNG hôte (native/compat/
// Arduino.h: random/randomSeed, déjà seedé par bootSeedRng() via
// radio_get_rng_seed() ci-dessous).
// ============================================================================
mesh::LocalIdentity radio_new_identity() {
  StdRNG rng;
  return mesh::LocalIdentity(&rng);
}

long radio_get_rng_seed() {
  return (long)::random(1, 0x7FFFFFFF);
}

void radio_set_params(float freq, float bw, uint8_t sf, uint8_t cr) {
  LOG_I("MESH-RADIO", "radio_set_params(%.3f, %.1f, %d, %d) — ignoré en simulateur", freq, bw, sf, cr);
}

void radio_set_tx_power(int8_t power_dbm) {
  LOG_I("MESH-RADIO", "radio_set_tx_power(%d) — ignoré en simulateur", power_dbm);
}

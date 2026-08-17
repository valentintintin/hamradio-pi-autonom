#include "target.h"

#include <Arduino.h>
#include <helpers/ArduinoHelpers.h>
#include "InternalRp2040RTCClock.h"
#include "SimWorld.h"
#include "core/Log.h"
#include "AprsRadioHwSim.h"
#include "AprsCarrierSim.h"
#include "ExternalRtcSim.h"

MyBoard board;

SimRadio mesh_radio_driver(SimWorld::instance().mesh_log, SimWorld::instance().mesh_rx_queue, "MESH-RADIO");
SimRadio aprs_radio_driver(SimWorld::instance().aprs_log, SimWorld::instance().aprs_rx_queue, "APRS-RADIO");

static AprsRadioHwSim aprs_radio_hw_sim;
static AprsCarrierSim aprs_carrier_sim;
IAprsRadioHw& aprs_radio_hw = aprs_radio_hw_sim;
IAprsCarrier& aprs_carrier = aprs_carrier_sim;

static ExternalRtcSim external_rtc_sim;
IExternalRtc& externalRtc = external_rtc_sim;

static InternalRp2040RTCClock fallback_clock;
AutoDiscoverRTCClock rtc_clock(fallback_clock);
SensorManager sensors;

bool mesh_radio_init() {
  rtc_clock.begin(Wire); // Wire natif : endTransmission() échoue toujours -> fallback systématique sur InternalRp2040RTCClock
  return true;
}

bool aprs_radio_init() {
  return true;
}

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

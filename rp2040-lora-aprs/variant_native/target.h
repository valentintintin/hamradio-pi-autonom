#pragma once

// ============================================================================
// RP-LoRA_Mini_v3_dual — variant target NATIF (env PlatformIO `native`)
//
// Même contrat que variant/target.h (mêmes symboles externs, mêmes fonctions
// radio_*) mais backé par SimRadio (native/sim/SimRadio.h) au lieu de
// RadioLib+SX1262 réels : main.cpp/core/Boot.cpp n'ont besoin d'aucune
// modification, ils ne connaissent ces objets qu'à travers leurs types
// abstraits (mesh::Radio, mesh::RTCClock...).
// ============================================================================

#include <helpers/AutoDiscoverRTCClock.h>
#include <helpers/SensorManager.h>
#include "SimRadio.h"
#include "MyBoard.h"
#include "aprs/AprsRadioHw.h"
#include "aprs/AprsCarrier.h"
#include "hal/rtc/ExternalRtc.h"

// --- MeshCore radio (868 MHz — simulée) -------------------------------------
extern SimRadio mesh_radio_driver;
extern AutoDiscoverRTCClock rtc_clock;
extern SensorManager sensors;
extern MyBoard board;

// --- APRS radio (433 MHz — simulée) -----------------------------------------
extern SimRadio aprs_radio_driver;

// Bascule LoRa/FSK + réception/relais WH65B (aprs_radio_hw) et séquence
// CW/SSTV (aprs_carrier) — mêmes noms/types que variant/target.h, backés ici
// par AprsRadioHwSim/AprsCarrierSim (SimWorld) au lieu de RadioLib+SX1262 réels.
extern IAprsRadioHw& aprs_radio_hw;
extern IAprsCarrier& aprs_carrier;

// Puce RTC externe — cf. hal/rtc/ExternalRtc.h. Pas de puce en natif
// (ExternalRtcSim, toujours absente) : l'horloge hôte fait déjà foi.
extern IExternalRtc& externalRtc;

// --- Init functions --------------------------------------------------------
bool mesh_radio_init();
bool aprs_radio_init();
mesh::LocalIdentity radio_new_identity();
long radio_get_rng_seed();
void radio_set_params(float freq, float bw, uint8_t sf, uint8_t cr);
void radio_set_tx_power(int8_t power_dbm);

// Aliases attendus par MeshCore (simple_repeater etc.)
#define radio_driver mesh_radio_driver
#define radio_init mesh_radio_init

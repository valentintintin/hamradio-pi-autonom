#pragma once

// ============================================================================
// RP-LoRA_Mini_v3_dual — variant target
// Board custom F4ISE : Pico W + 2x SX1262 (E22-900M30S + E22-400M33S)
// ============================================================================

#define RADIOLIB_STATIC_ONLY 1

#include <RadioLib.h>
#include <helpers/AutoDiscoverRTCClock.h>
#include <helpers/radiolib/CustomSX1262Wrapper.h>
#include <helpers/radiolib/RadioLibWrappers.h>
#include <helpers/SensorManager.h>
#include "MyBoard.h"
#include "aprs/AprsRadioHw.h"
#include "aprs/AprsCarrier.h"
#include "aprs/NotifyingRadioLibWrapper.h"
#include "hal/rtc/ExternalRtc.h"

// --- MeshCore radio (868 MHz, SPI1) ----------------------------------------
// MeshRadioWrapper (cf. aprs/NotifyingRadioLibWrapper.h) au lieu de
// WRAPPER_CLASS/CustomSX1262Wrapper directement : même comportement, en plus
// de réveiller task_mesh.cpp depuis l'IRQ DIO1 (cf. ce header) au lieu de
// laisser cette tâche faire du polling en vTaskDelay(1).
extern MeshRadioWrapper mesh_radio_driver;
extern AutoDiscoverRTCClock rtc_clock;
extern SensorManager sensors;
extern MyBoard board;

// --- APRS radio (433 MHz, SPI0) --------------------------------------------
// Utilise le meme wrapper RadioLib mais sur un autre SPI — AprsRadioWrapper,
// même remarque que mesh_radio_driver ci-dessus (réveille task_aprs.cpp).
extern CustomSX1262 aprs_radio_hw_sx1262;  // handle SX1262 concret (init bas niveau, cf. aprs_radio_init())
extern AprsRadioWrapper aprs_radio_driver;

// Bascule LoRa/FSK + réception/relais WH65B (aprs_radio_hw) et séquence
// CW/SSTV (aprs_carrier) : mêmes noms, deux implémentations (une par
// environnement, cf. AprsRadioHwReal/AprsCarrierReal ici et
// variant_native/AprsRadioHwSim/AprsCarrierSim) — aucun #ifdef NATIVE_BUILD
// nécessaire côté appelants (aprs/LoRa433RadioMode.cpp, tasks/task_weather.cpp,
// aprs/SstvTransmitter.cpp).
extern IAprsRadioHw& aprs_radio_hw;
extern IAprsCarrier& aprs_carrier;

// Puce RTC externe battery-backed (RX8025T, persistance entre coupures
// d'alimentation) — cf. hal/rtc/ExternalRtc.h.
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

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

// --- MeshCore radio (868 MHz, SPI1) ----------------------------------------
extern WRAPPER_CLASS mesh_radio_driver;
extern AutoDiscoverRTCClock rtc_clock;
extern SensorManager sensors;
extern MyBoard board;

// --- APRS radio (433 MHz, SPI0) --------------------------------------------
// Utilise le meme wrapper RadioLib mais sur un autre SPI
extern CustomSX1262 aprs_radio_hw;
extern CustomSX1262Wrapper aprs_radio_driver;

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

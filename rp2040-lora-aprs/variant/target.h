#pragma once

#define RADIOLIB_STATIC_ONLY 1

#include <RadioLib.h>
#include "InternalRp2040RTCClock.h"
#include <helpers/radiolib/CustomSX1262Wrapper.h>
#include <helpers/radiolib/RadioLibWrappers.h>
#include <helpers/SensorManager.h>
#include "MyBoard.h"
#include "aprs/AprsRadioHw.h"
#include "aprs/AprsCarrier.h"
#include "aprs/NotifyingRadioLibWrapper.h"
#include "hal/rtc/ExternalRtc.h"

extern MeshRadioWrapper mesh_radio_driver;
extern InternalRp2040RTCClock rtc_clock;
extern SensorManager sensors;
extern MyBoard board;

extern CustomSX1262 aprs_radio_hw_sx1262;
extern AprsRadioWrapper aprs_radio_driver;

extern IAprsRadioHw& aprs_radio_hw;
extern IAprsCarrier& aprs_carrier;

extern IExternalRtc& externalRtc;

bool mesh_radio_init();
bool aprs_radio_init();
mesh::LocalIdentity radio_new_identity();
long radio_get_rng_seed();
void radio_set_params(float freq, float bw, uint8_t sf, uint8_t cr);
void radio_set_tx_power(int8_t power_dbm);

// Aliases attendus par MeshCore (simple_repeater etc.)
#define radio_driver mesh_radio_driver
#define radio_init mesh_radio_init

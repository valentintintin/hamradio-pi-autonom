#pragma once

#include "pico/util/datetime.h"

// ============================================================================
// hardware/rtc.h — shim natif du RTC interne du RP2040, utilisé par
// variant/InternalRp2040RTCClock.h (fallback logiciel de AutoDiscoverRTCClock
// quand aucune puce RTC I2C n'est détectée — toujours le cas en natif, cf.
// native/compat/Wire.h). Backé par l'horloge murale de l'hôte (voir
// native/compat/rtc_compat.cpp) : donne une heure réaliste par défaut, sans
// code supplémentaire côté simulateur.
// ============================================================================

void rtc_init();
bool rtc_get_datetime(datetime_t* dt);
bool rtc_set_datetime(const datetime_t* dt);

#pragma once

#include <stdint.h>

// ============================================================================
// IExternalRtc — puce RTC externe I2C battery-backed (RX8025T sur cette
// carte), utilisée uniquement pour PERSISTER l'heure entre coupures
// d'alimentation : au boot, on lit son heure pour régler l'horloge interne
// RP2040 (cf. variant/target.cpp, mesh_radio_init()) ; à chaque "clockdate"
// (cf. cli/CommandHandler.cpp), on lui réécrit la nouvelle heure. Le reste du
// firmware continue de lire `rtc_clock` (AutoDiscoverRTCClock, cf.
// lib/MeshCore/src/helpers/AutoDiscoverRTCClock.h — DS3231/RV-3028/PCF8563/
// RX8130CE, qui ne connaît pas le RX8025T) comme avant.
//
// Deux implémentations, une par environnement (même principe que
// aprs/AprsRadioHw.h) : variant/ExternalRtcReal (vraie puce RX8025T via
// lib/Rx8025tRtc) et variant_native/ExternalRtcSim (no-op — rien à
// synchroniser en natif, l'horloge hôte fait déjà foi).
// ============================================================================

class IExternalRtc {
public:
  virtual ~IExternalRtc() = default;

  // Détecte la puce sur le bus I2C (déjà initialisé par l'appelant).
  virtual bool begin() = 0;

  virtual bool isPresent() const = 0;

  // Lit l'heure de la puce (epoch UTC). Retourne false si absente, ou si son
  // heure n'est pas fiable (VLF levé : alimentation coupée trop longtemps).
  virtual bool readTime(uint32_t* outUnixTime) = 0;

  // Écrit l'heure vers la puce (persistance batterie). Retourne false si absente.
  virtual bool writeTime(uint32_t unixTime) = 0;
};

#pragma once

#include <LittleFS.h>
#include <stdint.h>

// ============================================================================
// IAprsCarrier — séquence bloquante CW indicatif + SSTV + CW indicatif sur le
// SX1262 433 MHz, utilisée uniquement par aprs/SstvTransmitter.cpp.
//
// Séparée d'IAprsRadioHw (aprs/AprsRadioHw.h) : la génération de porteuse
// réelle (RadioLib MorseClient/SSTVClient) exige un pointeur PhysicalLayer*
// vers le vrai SX1262 — un détail d'implémentation propre à
// variant/AprsCarrierReal, qu'IAprsRadioHw n'a pas à exposer. SstvTransmitter.cpp
// ne connaît que cette interface, jamais RadioLib ni le SX1262 concret.
//
// Deux implémentations, une par environnement (cf. IAprsRadioHw) :
// variant/AprsCarrierReal (vraie émission RadioLib) et
// variant_native/AprsCarrierSim (séquence journalisée, aucune émission).
// ============================================================================

class IAprsCarrier {
public:
  virtual ~IAprsCarrier() = default;

  // `modeIndex` : index dans SSTV_MODE_LIST (cf. aprs/SstvTransmitter.h) —
  // l'implémentation réelle retrouve elle-même le SSTVMode_t RadioLib
  // correspondant (seule elle en a besoin, cf. commentaire ci-dessus).
  // `image` : fichier RGB888 déjà ouvert en lecture par l'appelant (largeur*
  // hauteur*3 octets, sans en-tête), positionné en tête.
  virtual void transmitCwSstv(const char* callsign, uint8_t cwRepeats, uint8_t cwWpm, float freqMhz,
                              int8_t powerDbm, uint8_t modeIndex, const char* modeName,
                              uint16_t width, uint16_t height, File& image) = 0;
};

#pragma once

#include "aprs/AprsCarrier.h"

// ============================================================================
// AprsCarrierSim — implémentation native d'IAprsCarrier (cf. aprs/AprsCarrier.h) :
// pas de SX1262 réel, donc pas de porteuse directe possible pour le CW/SSTV —
// la séquence est simulée par des logs (mêmes étapes que la version réelle,
// cf. variant/AprsCarrierReal), sans émission RF. L'image uploadée
// (LittleFS, déjà ouverte par l'appelant) est relue pour vérifier qu'elle est
// bien complète, mais son contenu n'est pas encodé/envoyé.
// ============================================================================

class AprsCarrierSim : public IAprsCarrier {
public:
  void transmitCwSstv(const char* callsign, uint8_t cwRepeats, uint8_t cwWpm, float freqMhz,
                      int8_t powerDbm, uint8_t modeIndex, const char* modeName,
                      uint16_t width, uint16_t height, File& image) override;
};

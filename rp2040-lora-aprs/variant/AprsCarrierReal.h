#pragma once

#include "aprs/AprsCarrier.h"
#include <helpers/radiolib/CustomSX1262Wrapper.h>

// ============================================================================
// AprsCarrierReal — implémentation matérielle d'IAprsCarrier (cf.
// aprs/AprsCarrier.h) : vraie émission CW+SSTV via RadioLib MorseClient/
// SSTVClient sur le SX1262 433 MHz. Contrepartie native :
// variant_native/AprsCarrierSim.h.
//
// Seule cette classe a besoin du pointeur RadioLib PhysicalLayer* (le SX1262
// concret) et de la table index->SSTVMode_t* — aprs/SstvTransmitter.cpp ne
// connaît que l'interface IAprsCarrier.
// ============================================================================

class AprsCarrierReal : public IAprsCarrier {
public:
  explicit AprsCarrierReal(CustomSX1262& hw) : _hw(hw) {}

  void transmitCwSstv(const char* callsign, uint8_t cwRepeats, uint8_t cwWpm, float freqMhz,
                      int8_t powerDbm, uint8_t modeIndex, const char* modeName,
                      uint16_t width, uint16_t height, File& image) override;

private:
  CustomSX1262& _hw;
};

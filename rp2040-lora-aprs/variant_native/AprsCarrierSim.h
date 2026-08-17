#pragma once

#include "aprs/AprsCarrier.h"

class AprsCarrierSim : public IAprsCarrier {
public:
  void transmitCwSstv(const char* callsign, uint8_t cwRepeats, uint8_t cwWpm, float freqMhz,
                      int8_t powerDbm, uint8_t modeIndex, const char* modeName,
                      uint16_t width, uint16_t height, File& image) override;
};

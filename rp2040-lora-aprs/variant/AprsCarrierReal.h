#pragma once

#include "aprs/AprsCarrier.h"
#include <helpers/radiolib/CustomSX1262Wrapper.h>

class AprsCarrierReal : public IAprsCarrier {
public:
  explicit AprsCarrierReal(CustomSX1262& hw) : _hw(hw) {}

  void transmitCwSstv(const char* callsign, uint8_t cwRepeats, uint8_t cwWpm, float freqMhz,
                      int8_t powerDbm, uint8_t modeIndex, const char* modeName,
                      uint16_t width, uint16_t height, File& image) override;

private:
  CustomSX1262& _hw;
};

#pragma once

#include "aprs/AprsRadioHw.h"
#include <helpers/radiolib/CustomSX1262Wrapper.h>

// ============================================================================
// AprsRadioHwReal — implémentation matérielle d'IAprsRadioHw (cf.
// aprs/AprsRadioHw.h), vrai SX1262 433 MHz via RadioLib. Contrepartie
// native : variant_native/AprsRadioHwSim.h.
// ============================================================================

class AprsRadioHwReal : public IAprsRadioHw {
public:
  explicit AprsRadioHwReal(CustomSX1262& hw) : _hw(hw) {}

  bool switchToFsk() override;
  bool switchToLora() override;
  bool receiveWh65bFrame(uint32_t timeoutMs, uint8_t* outBuf, float* outRssi) override;
  bool relayWh65bFrame(const uint8_t* data, size_t len, int8_t powerDbm) override;
  void standby() override { _hw.standby(); }

private:
  CustomSX1262& _hw;
};

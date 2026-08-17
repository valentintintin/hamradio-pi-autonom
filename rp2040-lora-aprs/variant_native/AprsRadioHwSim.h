#pragma once

#include "aprs/AprsRadioHw.h"

class AprsRadioHwSim : public IAprsRadioHw {
public:
  bool switchToFsk() override;
  bool switchToLora() override;
  bool receiveWh65bFrame(uint32_t timeoutMs, uint8_t* outBuf, float* outRssi) override;
  bool relayWh65bFrame(const uint8_t* data, size_t len, int8_t powerDbm) override;
  void standby() override {}
};

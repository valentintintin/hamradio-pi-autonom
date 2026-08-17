#pragma once

#include "hal/rtc/ExternalRtc.h"
#include <ctime>

class ExternalRtcSim : public IExternalRtc {
public:
  bool begin() override { return true; }
  bool isPresent() const override { return true; }
  bool readTime(uint32_t* outUnixTime) override {
    *outUnixTime = (uint32_t)time(nullptr);
    return true;
  }
  bool writeTime(uint32_t) override { return true; }
};

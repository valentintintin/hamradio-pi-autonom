#pragma once

#include "hal/rtc/ExternalRtc.h"
#include "hal/i2c/I2CBus.h"
#include <Rx8025t.h>

// Rx8025t (lib/Rx8025tRtc) parle directement à TwoWire sans verrou interne :
// le verrouillage via I2CBus se fait ici, autour de chaque appel.
class ExternalRtcReal : public IExternalRtc {
public:
  explicit ExternalRtcReal(I2CBus& bus) : _bus(&bus), _rtc(bus.wire()) {}

  bool begin() override;
  bool isPresent() const override { return _present; }
  bool readTime(uint32_t* outUnixTime) override;
  bool writeTime(uint32_t unixTime) override;

private:
  I2CBus* _bus;
  Rx8025t _rtc;
  bool _present = false;
};

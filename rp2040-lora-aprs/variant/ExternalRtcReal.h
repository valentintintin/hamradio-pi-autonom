#pragma once

#include "hal/rtc/ExternalRtc.h"
#include <Rx8025t.h>

// ============================================================================
// ExternalRtcReal — implémentation matérielle d'IExternalRtc (cf.
// hal/rtc/ExternalRtc.h) : vraie puce RX8025T via lib/Rx8025tRtc. Contrepartie
// native : variant_native/ExternalRtcSim.h.
// ============================================================================

class ExternalRtcReal : public IExternalRtc {
public:
  explicit ExternalRtcReal(TwoWire& wire) : _rtc(wire) {}

  bool begin() override;
  bool isPresent() const override { return _present; }
  bool readTime(uint32_t* outUnixTime) override;
  bool writeTime(uint32_t unixTime) override;

private:
  Rx8025t _rtc;
  bool _present = false;
};

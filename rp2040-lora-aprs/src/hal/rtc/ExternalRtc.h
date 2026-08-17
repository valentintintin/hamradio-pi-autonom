#pragma once

#include <stdint.h>

// RTC externe I2C (RX8025T). Volontairement pas AutoDiscoverRTCClock
// (lib/MeshCore) : son probe RX8130CE partage l'adresse I2C 0x32 du RX8025T
// mais pas son protocole, ce qui le ferait détecter à tort.

class IExternalRtc {
public:
  virtual ~IExternalRtc() = default;

  virtual bool begin() = 0;
  virtual bool isPresent() const = 0;

  // false si absente, ou si le flag VLF indique une heure non fiable
  // (alimentation coupée trop longtemps).
  virtual bool readTime(uint32_t* outUnixTime) = 0;
  virtual bool writeTime(uint32_t unixTime) = 0;
};

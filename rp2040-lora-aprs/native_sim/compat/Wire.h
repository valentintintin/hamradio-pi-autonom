#pragma once

#include <cstdint>
#include <cstddef>

// ============================================================================
// Wire.h — shim natif. Aucun bus I2C réel côté hôte : tous les capteurs/
// expandeurs/EEPROM sont simulés au niveau de leur HAL (cf. native/sim/,
// native/compat/Adafruit_*.h, TCA9555.h, VEDirect.h) sans jamais passer par
// TwoWire. endTransmission() renvoie systématiquement un échec (!= 0) :
// AutoDiscoverRTCClock (lib/MeshCore) retombe alors proprement sur son
// fallback logiciel sans code supplémentaire.
// ============================================================================

class TwoWire {
public:
  void begin() {}
  void end() {}
  void setSDA(int) {}
  void setSCL(int) {}

  void beginTransmission(uint8_t) {}
  uint8_t endTransmission(bool = true) { return 1; }

  size_t write(uint8_t) { return 0; }
  size_t write(const uint8_t*, size_t) { return 0; }

  size_t requestFrom(uint8_t, size_t, bool = true) { return 0; }
  int available() { return 0; }
  int read() { return -1; }
};

extern TwoWire Wire;
extern TwoWire Wire1;

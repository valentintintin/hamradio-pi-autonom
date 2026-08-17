#pragma once

#include <cstdint>
#include <cstddef>

class TwoWire {
public:
  void begin() {}
  void end() {}
  void setSDA(int) {}
  void setSCL(int) {}

  void beginTransmission(uint8_t) {}
  // Échec systématique voulu: force AutoDiscoverRTCClock (MeshCore) sur son
  // fallback logiciel plutôt que de simuler un vrai bus I2C.
  uint8_t endTransmission(bool = true) { return 1; }

  size_t write(uint8_t) { return 0; }
  size_t write(const uint8_t*, size_t) { return 0; }

  size_t requestFrom(uint8_t, size_t, bool = true) { return 0; }
  int available() { return 0; }
  int read() { return -1; }
};

extern TwoWire Wire;
extern TwoWire Wire1;

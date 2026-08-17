#pragma once

#include <stdint.h>

class GpioHal {
public:
  virtual ~GpioHal() = default;

  virtual bool begin() = 0;
  virtual bool set(bool level) = 0;
  virtual bool read(bool& level) = 0;

  // Défaut : set/delay/set. Les HAL dont la broche partage un registre avec
  // d'autres broches redéfinissent pour garantir l'atomicité de la séquence.
  virtual bool pulse(bool level, uint16_t ms);
};

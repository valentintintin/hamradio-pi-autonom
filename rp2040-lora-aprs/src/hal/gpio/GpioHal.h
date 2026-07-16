#pragma once

#include <stdint.h>

// ============================================================================
// GpioHal — interface commune à une broche digitale, que sa broche physique
// soit un GPIO natif du RP2040 (cf. PicoGpioHal) ou une broche d'un
// expandeur I2C (cf. Tca9555GpioHal). Le code métier (ex. RelayHal) manipule
// cette interface sans savoir sur quel support elle vit.
// ============================================================================

class GpioHal {
public:
  virtual ~GpioHal() = default;

  virtual bool begin() = 0;
  virtual bool set(bool level) = 0;
  virtual bool read(bool& level) = 0;

  // Porte la broche à `level` pendant `ms` puis la ramène à sa valeur
  // précédente. Implémentation par défaut : set/delay/set — les HAL dont la
  // broche partage une ressource (bus, registre) avec d'autres broches
  // redéfinissent cette méthode pour garantir l'atomicité de la séquence.
  virtual bool pulse(bool level, uint16_t ms);
};

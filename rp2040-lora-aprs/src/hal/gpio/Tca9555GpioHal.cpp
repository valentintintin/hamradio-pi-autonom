#include "Tca9555GpioHal.h"

bool Tca9555GpioHal::begin() {
  return _expander->setPinMode(_pin, _asOutput);
}

bool Tca9555GpioHal::set(bool level) {
  return _expander->writePin(_pin, level);
}

bool Tca9555GpioHal::read(bool& level) {
  return _expander->readPin(_pin, level);
}

bool Tca9555GpioHal::pulse(bool level, uint16_t ms) {
  // Redéfinie (plutôt que d'utiliser l'implémentation par défaut set/delay/set
  // de GpioHal) car le verrou I2C doit couvrir toute la séquence lue-modifiée-
  // écrite-attendue-réécrite : sinon une autre broche du même port, ou une
  // impulsion concurrente sur une autre broche, pourrait s'intercaler et
  // raccourcir l'impulsion en cours sous le temps de bascule minimal.
  return _expander->pulsePin(_pin, level, ms);
}

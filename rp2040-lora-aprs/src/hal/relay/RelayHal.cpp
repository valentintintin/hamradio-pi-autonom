#include "RelayHal.h"
#include "hal/gpio/Tca9555GpioHal.h"

#define RELAY_PULSE_MS 150

bool RelayHal::begin(Settings::Relay* channels, uint8_t count) {
  _channels = channels;
  _count = count;

  // Écrire le registre de SORTIE avant de basculer les broches en sortie : le
  // TCA9555 démarre avec ses 8 broches en entrée et son registre de sortie à
  // 0xFF (valeur usine). Si on inversait l'ordre et que l'écriture du
  // registre de sortie échouait juste après avoir activé les sorties (config
  // écrite, output pas encore), les 8 lignes des 4 relais se retrouveraient
  // à l'état haut sans retour possible.
  bool output_ok = _expander->writeOutputPort(TCA9555_RELAY_PORT, 0x00);
  _initialized = output_ok && _expander->writeConfigPort(TCA9555_RELAY_PORT, 0x00);
  return _initialized;
}

bool RelayHal::setState(uint8_t index, bool on) {
  if (!_initialized || index >= _count) {
    return false;
  }

  uint8_t bit = index * 2 + (on ? 0 : 1); // pair = ligne "set", impair = ligne "reset"

  // Passe par l'abstraction GpioHal générique plutôt que par le registre
  // TCA9555 directement : RelayHal ignore si sa broche vit sur l'expandeur
  // I2C ou (demain) sur un GPIO natif du RP2040.
  Tca9555GpioHal pin(*_expander, bit, /*asOutput=*/true);
  pin.pulse(true, RELAY_PULSE_MS);

  _channels[index].state = on;
  return true;
}

bool RelayHal::getState(uint8_t index) const {
  return (index < _count) ? _channels[index].state : false;
}

#include "RelayHal.h"
#include "hal/gpio/Tca9555GpioHal.h"
#include "core/ScratchRegisters.h"
#include <Arduino.h>
#include <hardware/structs/watchdog.h>

#define RELAY_PULSE_MS 150

bool RelayHal::begin(uint8_t count) {
  _count = count;

  // TCA9555 démarre avec les broches en entrée et le registre de sortie à
  // 0xFF (valeur usine) : écrire la sortie avant de configurer la direction,
  // sinon un échec entre les deux laisserait les 4 relais à l'état haut sans
  // retour possible.
  bool output_ok = _expander->writeOutputPort(TCA9555_RELAY_PORT, 0x00);
  _initialized = output_ok && _expander->writeConfigPort(TCA9555_RELAY_PORT, 0x00);

  // Note : le guard sur WDT_RESET est désactivé (commenté) ci-dessous — l'état
  // scratch est donc restauré à chaque boot, pas seulement après un reset
  // watchdog. À vérifier si ce n'est pas voulu.
  uint32_t mask = watchdog_hw->scratch[SCRATCH_RELAY_STATE_MASK];
  for (uint8_t i = 0; i < _count; i++) {
    _state[i] = (mask & (1u << i)) != 0;
  }

  return _initialized;
}

bool RelayHal::setState(uint8_t index, bool on) {
  if (!_initialized || index >= _count) {
    return false;
  }

  uint8_t bit = index * 2 + (on ? 0 : 1); // pair = ligne "set", impair = ligne "reset"

  Tca9555GpioHal pin(*_expander, bit, /*asOutput=*/true);
  pin.pulse(true, RELAY_PULSE_MS);

  _state[index] = on;

  if (on) {
    watchdog_hw->scratch[SCRATCH_RELAY_STATE_MASK] |= (1u << index);
  } else {
    watchdog_hw->scratch[SCRATCH_RELAY_STATE_MASK] &= ~(1u << index);
  }

  return true;
}

bool RelayHal::getState(uint8_t index) const {
  return (index < _count) && _state[index];
}

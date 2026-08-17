#include "RelayHal.h"
#include "hal/gpio/Tca9555GpioHal.h"
#include "core/ScratchRegisters.h"
#include <Arduino.h>
#include <hardware/structs/watchdog.h>

#define RELAY_PULSE_MS 150

bool RelayHal::begin(uint8_t count) {
  _count = count;

  // Écrire le registre de SORTIE avant de basculer les broches en sortie : le
  // TCA9555 démarre avec ses 8 broches en entrée et son registre de sortie à
  // 0xFF (valeur usine). Si on inversait l'ordre et que l'écriture du
  // registre de sortie échouait juste après avoir activé les sorties (config
  // écrite, output pas encore), les 8 lignes des 4 relais se retrouveraient
  // à l'état haut sans retour possible.
  bool output_ok = _expander->writeOutputPort(TCA9555_RELAY_PORT, 0x00);
  _initialized = output_ok && _expander->writeConfigPort(TCA9555_RELAY_PORT, 0x00);

  // Restaure l'état connu depuis le registre watchdog scratch (cf.
  // core/ScratchRegisters.h — always-on, survit à un reset RP2040 "à chaud")
  // — uniquement si CE reset en est un : WDT_RESET est vrai aussi bien après
  // un vrai timeout watchdog qu'après rp2040.reboot() ("reboot"/"dfu", qui
  // passe par watchdog_reboot() — cf. framework-arduinopico/RP2040Support.h).
  // Après une vraie coupure d'alimentation (POR/BOR efface ce registre),
  // WDT_RESET ne peut pas être vrai : on repart alors simplement à "éteint"
  // (défaut de _state[], sans conséquence — cf. Settings::Relay).
  //if (rp2040.getResetReason() == RP2040::WDT_RESET) {
    uint32_t mask = watchdog_hw->scratch[SCRATCH_RELAY_STATE_MASK];
    for (uint8_t i = 0; i < _count; i++) {
      _state[i] = (mask & (1u << i)) != 0;
    //}
  }

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

  _state[index] = on;

  // Mirroir dans le scratch watchdog (cf. begin()) : seul point de passage
  // de tout changement matériel (setManualState() appelle cette méthode).
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

bool RelayHal::setManualState(uint8_t index, bool on) {
  bool ok = setState(index, on);
  if (index < _count) {
    _manual_override[index] = true;
  }
  return ok;
}

void RelayHal::clearManualOverride(uint8_t index) {
  if (index < _count) {
    _manual_override[index] = false;
  }
}

bool RelayHal::isManualOverride(uint8_t index) const {
  return (index < _count) && _manual_override[index];
}

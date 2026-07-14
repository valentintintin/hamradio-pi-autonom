#include "RelayHal.h"
#include <Arduino.h>

#define RELAY_PULSE_MS   50
#define TCA9555_REG_OUTPUT_P0 0x02
#define TCA9555_REG_CONFIG_P0 0x06

bool RelayHal::begin(RelayChannel* channels, uint8_t count) {
  _channels = channels;
  _count = count;

  // Écrire le registre de SORTIE avant de basculer les broches en sortie : le
  // TCA9555 démarre avec ses 8 broches en entrée et son registre de sortie à
  // 0xFF (valeur usine). Si on inversait l'ordre et que l'écriture du
  // registre de sortie échouait juste après avoir activé les sorties (config
  // écrite, output pas encore), les 8 lignes des 4 relais se retrouveraient
  // à l'état haut sans retour possible.
  bool output_ok = writeRegister(TCA9555_REG_OUTPUT_P0, 0x00);
  _initialized = output_ok && writeRegister(TCA9555_REG_CONFIG_P0, 0x00);
  return _initialized;
}

bool RelayHal::writeRegister(uint8_t reg, uint8_t value) {
  if (!_bus->lock()) {
    return false;
  }
  _bus->wire().beginTransmission(_addr);
  _bus->wire().write(reg);
  _bus->wire().write(value);
  bool ok = (_bus->wire().endTransmission() == 0);
  _bus->unlock();
  return ok;
}

void RelayHal::pulseBit(uint8_t bit) {
  // Le verrou I2C doit couvrir toute l'impulsion (pas juste chaque écriture
  // séparément) : sinon deux setState() concomitants sur deux relais
  // différents peuvent chacun écraser le registre de sortie de l'autre en
  // plein milieu de son impulsion (le registre représente les 8 lignes à la
  // fois), ce qui peut couper une impulsion avant le temps de bascule minimal
  // du relais bistable, sans qu'aucune erreur ne remonte.
  if (!_bus->lock()) {
    return;
  }

  uint8_t mask = (uint8_t)(1 << bit);

  _bus->wire().beginTransmission(_addr);
  _bus->wire().write(TCA9555_REG_OUTPUT_P0);
  _bus->wire().write(mask); // impulsion : ce bit haut, tous les autres bas
  _bus->wire().endTransmission();

  delay(RELAY_PULSE_MS);

  _bus->wire().beginTransmission(_addr);
  _bus->wire().write(TCA9555_REG_OUTPUT_P0);
  _bus->wire().write((uint8_t)0x00); // retour au repos
  _bus->wire().endTransmission();

  _bus->unlock();
}

bool RelayHal::setState(uint8_t index, bool on) {
  if (!_initialized || index >= _count) {
    return false;
  }

  uint8_t bit = index * 2 + (on ? 0 : 1); // pair = ligne "set", impair = ligne "reset"
  pulseBit(bit);
  _channels[index].state = on;
  return true;
}

bool RelayHal::getState(uint8_t index) const {
  return (index < _count) ? _channels[index].state : false;
}

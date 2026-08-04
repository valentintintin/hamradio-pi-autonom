#pragma once

#include "config/Settings.h"
#include "hal/i2c/Tca9555Hal.h"
#include <stdint.h>

// ============================================================================
// RelayHal — pilotage de 4 relais bistables via l'expandeur I2C TCA9555
// (carte "Interface" F1ZIC, bus I2C0 partagé avec BME280/INA3221/EEPROM/MPPT)
//
// Un relais bistable bascule d'état avec une brève impulsion sur une des
// deux lignes de commande (set/reset) et ne consomme aucun courant de
// maintien — l'état mécanique est conservé sans alimentation, y compris
// après un reboot.
//
// Mapping port 0 du TCA9555 (cf. SCH_Interface_2026-07-01.pdf) :
//   P00/P01 = relais 1 set/reset      P04/P05 = relais 3 set/reset
//   P02/P03 = relais 2 set/reset      P06/P07 = relais 4 set/reset
// Port 0 entièrement dédié aux relais : repos = tous les bits bas (aucune
// ligne maintenue haute), donc pas de read-modify-write nécessaire pour
// l'initialisation du port (cf. begin()).
//
// L'expandeur est créé et possédé par l'appelant (cf. main.cpp) — comme
// I2CBus, c'est une ressource partagée sur le bus I2C0, pas une propriété du
// RelayHal.
// ============================================================================

#define TCA9555_RELAY_ADDR 0x20
#define TCA9555_RELAY_PORT 0

class RelayHal {
public:
  explicit RelayHal(Tca9555Hal& expander)
    : _expander(&expander), _initialized(false) {}

  // `channels` doit pointer vers settings.relay (persisté), `count` = RELAY_COUNT
  bool begin(Settings::Relay* channels, uint8_t count);

  // Bascule le relais `index` (impulsion I2C) et persiste le nouvel état.
  bool setState(uint8_t index, bool on);
  bool getState(uint8_t index) const;
  bool isInitialized() const { return _initialized; }

private:
  Tca9555Hal* _expander;
  bool _initialized;
  Settings::Relay* _channels = nullptr;
  uint8_t _count = 0;
};

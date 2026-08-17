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

  // `count` = RELAY_COUNT. L'état ON/OFF n'est pas une settings persistée
  // (cf. Settings::Relay) : begin() le restaure depuis un registre watchdog
  // scratch (always-on, cf. core/ScratchRegisters.h) si ce reset est "à
  // chaud" (watchdog/reboot()), sinon repart à "éteint" (vraie coupure
  // d'alimentation — le relais physique bistable, lui, n'a pas bougé).
  bool begin(uint8_t count);

  // Bascule le relais `index` (impulsion I2C) et met à jour l'état connu
  // (RAM + mirroir scratch, cf. begin()). Utilisée par l'automatisme
  // (Relay::updateCutoff/updatePeriodic) — ne touche jamais le flag de
  // contrôle manuel (cf. setManualState() plus bas).
  bool setState(uint8_t index, bool on);
  bool getState(uint8_t index) const;
  bool isInitialized() const { return _initialized; }

  // --- Contrôle manuel (override) -------------------------------------------
  // Un utilisateur qui commande explicitement un relais (CLI "relay N on/off",
  // message APRS privilégié...) doit garder la main : plus aucune action de
  // Relay::updateCutoff/updatePeriodic sur ce relais tant que
  // clearManualOverride() n'a pas été appelé explicitement (cf. commande CLI
  // "relay N auto"). Volatile (RAM uniquement, pas dans Settings::Relay) —
  // un reboot repart toujours en automatique, par choix produit.
  bool setManualState(uint8_t index, bool on);
  void clearManualOverride(uint8_t index);
  bool isManualOverride(uint8_t index) const;

private:
  Tca9555Hal* _expander;
  bool _initialized;
  uint8_t _count = 0;
  bool _state[RELAY_COUNT] = {false, false, false, false};
  bool _manual_override[RELAY_COUNT] = {false, false, false, false};
};

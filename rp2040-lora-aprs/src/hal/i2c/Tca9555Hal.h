#pragma once

#include "I2CBus.h"
#include <TCA9555.h>
#include <stdint.h>

// ============================================================================
// Tca9555Hal — accès à l'expandeur I2C TCA9555, via la lib RobTillaart/TCA9555
// (https://github.com/RobTillaart/TCA9555, dispo dans le registre PlatformIO)
// pour le protocole registre. N'ajoute que ce que la lib ne fait pas :
// le verrouillage I2CBus (thread-safety FreeRTOS) autour de chaque opération.
// ============================================================================

class Tca9555Hal {
public:
  Tca9555Hal(I2CBus& bus, uint8_t addr)
    : _bus(&bus), _tca(addr, &bus.wire()) {}

  bool isConnected();

  // Écrit le registre de sortie brut du port `port` (0 ou 1) en une seule
  // transaction — pour initialiser un port entièrement dédié (cf.
  // RelayHal::begin, qui doit écrire la sortie avant de configurer la
  // direction, sans passer par un read-modify-write inutile).
  bool writeOutputPort(uint8_t port, uint8_t value);

  // Écrit le registre de config brut du port `port` (0 = sortie, 1 = entrée
  // par bit), même usage que writeOutputPort.
  bool writeConfigPort(uint8_t port, uint8_t configMask);

  // API par broche (0-15) : lecture-modification-écriture faite par la lib,
  // les 7 autres broches du port ne sont pas affectées.
  bool setPinMode(uint8_t pin, bool asOutput);
  bool writePin(uint8_t pin, bool level);
  bool readPin(uint8_t pin, bool& level);

  // Porte `pin` à `level` pendant `ms` puis la ramène à sa valeur d'avant
  // l'impulsion, verrou I2C tenu sur toute la séquence (écriture, delay,
  // écriture retour) pour qu'aucune autre transaction sur ce port ne puisse
  // s'intercaler et raccourcir l'impulsion.
  bool pulsePin(uint8_t pin, bool level, uint16_t ms);

private:
  I2CBus* _bus;
  TCA9555 _tca;
};

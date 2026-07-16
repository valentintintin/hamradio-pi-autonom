#pragma once

#include "hal/I2CBus.h"
#include <stdint.h>

// ============================================================================
// Tca9555Hal — accès registre bas niveau à l'expandeur I2C TCA9555 (16 GPIO
// répartis sur 2 ports de 8 bits). Ne connaît rien du câblage ni de l'usage
// des broches : cette logique métier vit dans les HAL qui l'utilisent
// (ex. RelayHal).
// ============================================================================

#define TCA9555_REG_INPUT_P0    0x00
#define TCA9555_REG_INPUT_P1    0x01
#define TCA9555_REG_OUTPUT_P0   0x02
#define TCA9555_REG_OUTPUT_P1   0x03
#define TCA9555_REG_POLARITY_P0 0x04
#define TCA9555_REG_POLARITY_P1 0x05
#define TCA9555_REG_CONFIG_P0   0x06
#define TCA9555_REG_CONFIG_P1   0x07

class Tca9555Hal {
public:
  Tca9555Hal(I2CBus& bus, uint8_t addr)
    : _bus(&bus), _addr(addr) {}

  // Écrit un registre en une transaction I2C verrouillée.
  bool writeRegister(uint8_t reg, uint8_t value);

  // Lit un registre en une transaction I2C verrouillée.
  bool readRegister(uint8_t reg, uint8_t& value);

  // Variantes sans verrou, pour les appelants qui doivent enchaîner plusieurs
  // écritures sous un même lock (ex. impulsion set/reset d'un relais
  // bistable, où le verrou doit couvrir toute la séquence).
  bool writeRegisterNoLock(uint8_t reg, uint8_t value);
  bool readRegisterNoLock(uint8_t reg, uint8_t& value);

  bool writeOutputPort(uint8_t port, uint8_t value) {
    return writeRegister(TCA9555_REG_OUTPUT_P0 + port, value);
  }

  bool writeConfigPort(uint8_t port, uint8_t value) {
    return writeRegister(TCA9555_REG_CONFIG_P0 + port, value);
  }

  bool readInputPort(uint8_t port, uint8_t& value) {
    return readRegister(TCA9555_REG_INPUT_P0 + port, value);
  }

  // API par broche (0-15, port = pin/8, bit = pin%8) : lecture-modification-
  // écriture du registre concerné, les 7 autres broches du port ne sont pas
  // affectées. Utilisée par Tca9555GpioHal pour exposer une broche du TCA9555
  // comme un GpioHal générique.
  bool setPinMode(uint8_t pin, bool asOutput);
  bool writePin(uint8_t pin, bool level);
  bool readPin(uint8_t pin, bool& level);

  // Porte `pin` à `level` pendant `ms` puis la ramène à sa valeur d'avant
  // l'impulsion, verrou I2C tenu sur toute la séquence (écriture, delay,
  // écriture retour) pour qu'aucune autre transaction sur ce port ne puisse
  // s'intercaler et raccourcir l'impulsion.
  bool pulsePin(uint8_t pin, bool level, uint16_t ms);

  // Accès au bus partagé pour les appelants qui ont besoin de tenir le
  // verrou sur plusieurs appels (cf. writeRegisterNoLock).
  I2CBus& bus() { return *_bus; }

private:
  I2CBus* _bus;
  uint8_t _addr;
};

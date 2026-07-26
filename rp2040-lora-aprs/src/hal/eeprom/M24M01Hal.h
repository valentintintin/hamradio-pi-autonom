#pragma once

#include "hal/i2c/I2CBus.h"
#include <stdint.h>
#include <stddef.h>

// ============================================================================
// M24M01Hal — EEPROM I2C M24M01 (ST), 1 Mbit / 128 Ko, adresse 0x50.
// Persistance des settings (cf. SettingsManager) et historique télémétrie
// (cf. TelemetryHistory).
//
// Adressage 17 bits (128 Ko = 2^17) : les 16 bits de poids faible passent
// dans les 2 octets d'adresse classiques ; le bit A16 (MSB) est replié dans
// le bit 0 du code de sélection du device I2C — cf. datasheet M24M01-R,
// "Device select code" = 1010 E2 E1 A16, avec E2/E1 câblés à la masse ici.
// ============================================================================

#define M24M01_I2C_ADDR   0x50
#define M24M01_PAGE_SIZE  256
#define M24M01_SIZE_BYTES (128 * 1024)  // 1 Mbit = 128 Ko

class M24M01Hal {
public:
  M24M01Hal(I2CBus& bus, uint8_t addr = M24M01_I2C_ADDR)
    : _bus(&bus), _addr(addr), _initialized(false) {}

  bool begin();

  // Lire un bloc (découpe en chunks selon la limite du buffer Wire)
  bool read(uint32_t address, uint8_t* data, size_t len);

  // Écrire un bloc (découpe en pages de 256 bytes puis en chunks Wire)
  bool write(uint32_t address, const uint8_t* data, size_t len);

  bool isInitialized() const { return _initialized; }

private:
  I2CBus* _bus;
  uint8_t _addr;
  bool _initialized;

  uint8_t deviceAddrFor(uint32_t address) const {
    return _addr | ((address >> 16) & 0x01);
  }

  bool readChunk(uint32_t address, uint8_t* data, size_t len);
  bool writePage(uint32_t address, const uint8_t* data, size_t len);

  // Attend la fin du cycle d'écriture en cours par polling ACK (cf. datasheet
  // §"Acknowledge polling") plutôt qu'un délai fixe : plus rapide dans le cas
  // typique, et sûr même si le cycle dépasse la durée typique (tW max = 5 ms).
  bool waitReady();
};

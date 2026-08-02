#pragma once

#include "hal/i2c/I2CBus.h"
#include <stdint.h>
#include <stddef.h>

// ============================================================================
// M24M01Hal — EEPROM I2C M24M01 (ST), 1 Mbit / 128 Ko, adresse 0x50.
// Persistance des settings (cf. SettingsManager) et historique télémétrie
// (cf. TelemetryHistory).
//
// L'implémentation réelle (M24M01Hal.cpp) délègue à la lib externe M24M01
// (github.com/valentintintin/M24M01), en ajoutant le verrouillage du bus
// (I2CBus, mutex FreeRTOS partagé avec les autres périphériques I2C) autour
// de chaque appel. La classe M24M01 n'est que forward-déclarée ici : la
// variante native (native_sim/sim/M24M01HalSim.cpp, persistée dans de vrais
// fichiers JSON, pas de bus I2C réel) ne s'en sert pas et n'a donc pas besoin
// de cette dépendance.
// ============================================================================

#define M24M01_I2C_ADDR   0x50
#define M24M01_PAGE_SIZE  256
#define M24M01_SIZE_BYTES (128 * 1024)  // 1 Mbit = 128 Ko

class M24M01;

class M24M01Hal {
public:
  M24M01Hal(I2CBus& bus, uint8_t addr = M24M01_I2C_ADDR)
    : _bus(&bus), _addr(addr), _initialized(false), _dev(nullptr) {}

  bool begin();

  // Lire un bloc (délégué à M24M01::read, cf. M24M01Hal.cpp)
  bool read(uint32_t address, uint8_t* data, size_t len);

  // Écrire un bloc (délégué à M24M01::write, cf. M24M01Hal.cpp)
  bool write(uint32_t address, const uint8_t* data, size_t len);

  bool isInitialized() const { return _initialized; }

private:
  I2CBus* _bus;
  uint8_t _addr;
  bool _initialized;
  M24M01* _dev;  // alloué par M24M01Hal.cpp::begin()
};

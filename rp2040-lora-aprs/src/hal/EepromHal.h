#pragma once

#include "I2CBus.h"
#include <stdint.h>
#include <string.h>

// ============================================================================
// M24M01 EEPROM HAL — 128KB I2C EEPROM (addr 0x50)
// Lecture/écriture de blocs pour persistance settings
// ============================================================================

#define EEPROM_I2C_ADDR     0x50
#define EEPROM_PAGE_SIZE    256
#define EEPROM_SIZE_BYTES   (128 * 1024)  // 1 Mbit = 128 KB

class EepromHal {
public:
  EepromHal(I2CBus& bus, uint8_t addr = EEPROM_I2C_ADDR)
    : _bus(&bus), _addr(addr), _initialized(false) {}

  bool begin() {
    if (!_bus->lock()) return false;
    // Test de communication : lire 1 byte à l'adresse 0
    _bus->wire().beginTransmission(_addr);
    _bus->wire().write((uint8_t)0); // addr high
    _bus->wire().write((uint8_t)0); // addr low
    _initialized = (_bus->wire().endTransmission() == 0);
    _bus->unlock();
    return _initialized;
  }

  // Lire un bloc (max 256 bytes par appel pour rester dans les limites Wire)
  bool read(uint32_t address, uint8_t* data, size_t len) {
    if (!_initialized || address + len > EEPROM_SIZE_BYTES) return false;
    if (!_bus->lock()) return false;

    // M24M01 : bit 17 de l'adresse dans le bit 0 de l'adresse I2C
    uint8_t dev_addr = _addr | ((address >> 16) & 0x01);

    _bus->wire().beginTransmission(dev_addr);
    _bus->wire().write((uint8_t)(address >> 8));
    _bus->wire().write((uint8_t)(address & 0xFF));

    if (_bus->wire().endTransmission(false) != 0) {
      _bus->unlock();
      return false;
    }

    size_t received = _bus->wire().requestFrom(dev_addr, (uint8_t)len);
    for (size_t i = 0; i < received; i++) {
      data[i] = _bus->wire().read();
    }

    _bus->unlock();
    return received == len;
  }

  // Écrire un bloc (gère le découpage en pages)
  bool write(uint32_t address, const uint8_t* data, size_t len) {
    if (!_initialized || address + len > EEPROM_SIZE_BYTES) return false;

    size_t offset = 0;
    while (offset < len) {
      // Calculer combien on peut écrire dans la page courante
      size_t page_offset = (address + offset) % EEPROM_PAGE_SIZE;
      size_t chunk = EEPROM_PAGE_SIZE - page_offset;
      if (chunk > len - offset) chunk = len - offset;
      // Wire buffer limit
      if (chunk > 30) chunk = 30;

      if (!writePage(address + offset, data + offset, chunk)) return false;
      offset += chunk;

      // Attendre l'écriture (5ms typique M24M01)
      delay(6);
    }
    return true;
  }

  bool isInitialized() const { return _initialized; }

private:
  I2CBus* _bus;
  uint8_t _addr;
  bool _initialized;

  bool writePage(uint32_t address, const uint8_t* data, size_t len) {
    if (!_bus->lock()) return false;

    uint8_t dev_addr = _addr | ((address >> 16) & 0x01);

    _bus->wire().beginTransmission(dev_addr);
    _bus->wire().write((uint8_t)(address >> 8));
    _bus->wire().write((uint8_t)(address & 0xFF));
    _bus->wire().write(data, len);
    bool ok = (_bus->wire().endTransmission() == 0);

    _bus->unlock();
    return ok;
  }
};

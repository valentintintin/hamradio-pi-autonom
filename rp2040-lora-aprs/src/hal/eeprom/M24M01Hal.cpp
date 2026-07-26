#include "M24M01Hal.h"
#include <Arduino.h>

#define M24M01_I2C_CHUNK_MAX     30 // limite buffer Wire (32) moins les 2 bytes d'adresse
#define M24M01_WRITE_TIMEOUT_MS  10 // tW max = 5 ms (datasheet) + marge

bool M24M01Hal::begin() {
  if (!_bus->lock()) {
    return false;
  }
  _bus->wire().beginTransmission(_addr);
  _initialized = (_bus->wire().endTransmission() == 0);
  _bus->unlock();
  return _initialized;
}

bool M24M01Hal::waitReady() {
  uint32_t start = millis();
  do {
    _bus->wire().beginTransmission(_addr);
    if (_bus->wire().endTransmission() == 0) {
      return true;
    }
  } while (millis() - start < M24M01_WRITE_TIMEOUT_MS);
  return false;
}

bool M24M01Hal::readChunk(uint32_t address, uint8_t* data, size_t len) {
  uint8_t dev_addr = deviceAddrFor(address);

  _bus->wire().beginTransmission(dev_addr);
  _bus->wire().write((uint8_t)(address >> 8));
  _bus->wire().write((uint8_t)(address & 0xFF));
  if (_bus->wire().endTransmission(false) != 0) {
    return false;
  }

  size_t received = _bus->wire().requestFrom(dev_addr, (uint8_t)len);
  for (size_t i = 0; i < received; i++) {
    data[i] = _bus->wire().read();
  }
  return received == len;
}

bool M24M01Hal::read(uint32_t address, uint8_t* data, size_t len) {
  if (!_initialized || address + len > M24M01_SIZE_BYTES) {
    return false;
  }

  size_t offset = 0;
  while (offset < len) {
    size_t chunk = len - offset;
    if (chunk > M24M01_I2C_CHUNK_MAX) {
      chunk = M24M01_I2C_CHUNK_MAX;
    }

    if (!_bus->lock()) {
      return false;
    }
    bool ok = waitReady() && readChunk(address + offset, data + offset, chunk);
    _bus->unlock();
    if (!ok) {
      return false;
    }

    offset += chunk;
  }
  return true;
}

bool M24M01Hal::writePage(uint32_t address, const uint8_t* data, size_t len) {
  if (!_bus->lock()) {
    return false;
  }

  bool ok = waitReady();
  if (ok) {
    uint8_t dev_addr = deviceAddrFor(address);
    _bus->wire().beginTransmission(dev_addr);
    _bus->wire().write((uint8_t)(address >> 8));
    _bus->wire().write((uint8_t)(address & 0xFF));
    _bus->wire().write(data, len);
    ok = (_bus->wire().endTransmission() == 0);
  }

  _bus->unlock();
  return ok;
}

bool M24M01Hal::write(uint32_t address, const uint8_t* data, size_t len) {
  if (!_initialized || address + len > M24M01_SIZE_BYTES) {
    return false;
  }

  size_t offset = 0;
  while (offset < len) {
    // Calculer combien on peut écrire dans la page courante
    size_t page_offset = (address + offset) % M24M01_PAGE_SIZE;
    size_t chunk = M24M01_PAGE_SIZE - page_offset;
    if (chunk > len - offset) {
      chunk = len - offset;
    }
    if (chunk > M24M01_I2C_CHUNK_MAX) {
      chunk = M24M01_I2C_CHUNK_MAX;
    }

    if (!writePage(address + offset, data + offset, chunk)) {
      return false;
    }
    offset += chunk;
  }
  return true;
}

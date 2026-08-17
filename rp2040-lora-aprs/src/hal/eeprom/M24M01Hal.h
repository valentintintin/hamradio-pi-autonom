#pragma once

#include "hal/i2c/I2CBus.h"
#include <stdint.h>
#include <stddef.h>

#define M24M01_I2C_ADDR   0x50
#define M24M01_PAGE_SIZE  256
#define M24M01_SIZE_BYTES (128 * 1024)  // 1 Mbit = 128 Ko

class M24M01;

class M24M01Hal {
public:
  M24M01Hal(I2CBus& bus, uint8_t addr = M24M01_I2C_ADDR)
    : _bus(&bus), _addr(addr), _initialized(false), _dev(nullptr) {}

  bool begin();
  bool read(uint32_t address, uint8_t* data, size_t len);
  bool write(uint32_t address, const uint8_t* data, size_t len);

  bool isInitialized() const { return _initialized; }

private:
  I2CBus* _bus;
  uint8_t _addr;
  bool _initialized;
  M24M01* _dev;
};

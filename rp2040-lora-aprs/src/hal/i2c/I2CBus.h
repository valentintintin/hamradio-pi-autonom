#pragma once

#include <Wire.h>
#include <FreeRTOS.h>
#include <semphr.h>

class I2CBus {
public:
  I2CBus(TwoWire& wire, int sda, int scl)
    : _wire(&wire), _sda(sda), _scl(scl), _mutex(nullptr) {}

  void begin() {
    _mutex = xSemaphoreCreateMutex();
    _wire->setSDA(_sda);
    _wire->setSCL(_scl);
    _wire->begin();
  }

  bool lock(TickType_t timeout = pdMS_TO_TICKS(100)) {
    return xSemaphoreTake(_mutex, timeout) == pdTRUE;
  }

  void unlock() {
    xSemaphoreGive(_mutex);
  }

  TwoWire& wire() { return *_wire; }

private:
  TwoWire* _wire;
  int _sda, _scl;
  SemaphoreHandle_t _mutex;
};

#pragma once

#include "config/Settings.h"
#include "../gpio/Tca9555Hal.h"
#include <stdint.h>

// 4 relais bistables sur l'expandeur I2C TCA9555 : une brève impulsion sur
// une des deux lignes (set/reset) bascule l'état, sans courant de maintien —
// l'état mécanique survit à une coupure d'alimentation.
//
// Mapping port 0 (cf. SCH_Interface_2026-07-01.pdf) :
//   P00/P01 = relais 1 set/reset      P04/P05 = relais 3 set/reset
//   P02/P03 = relais 2 set/reset      P06/P07 = relais 4 set/reset

#define TCA9555_RELAY_ADDR 0x20
#define TCA9555_RELAY_PORT 0

class RelayHal {
public:
  explicit RelayHal(Tca9555Hal& expander)
    : _expander(&expander), _initialized(false) {}

  bool begin(uint8_t count);
  bool setState(uint8_t index, bool on);
  bool getState(uint8_t index) const;
  bool isInitialized() const { return _initialized; }

private:
  Tca9555Hal* _expander;
  bool _initialized;
  uint8_t _count = 0;
  bool _state[RELAY_COUNT] = {false, false, false, false};
};

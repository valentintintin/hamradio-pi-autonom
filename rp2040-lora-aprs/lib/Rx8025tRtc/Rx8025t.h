#pragma once

// ============================================================================
// Rx8025t — pilote minimal pour l'horloge RTC Epson RX8025T (I2C, addr 0x32).
//
// Adapté de https://github.com/45gfg9/RTClib — cf. LICENSE dans ce dossier
// pour la licence complète (MIT) et le détail de ce qui a été repris.
// ============================================================================

#include <Arduino.h>
#include <Wire.h>
#include <time.h>

class Rx8025t {
public:
  static constexpr uint8_t ADDRESS = 0x32;

  explicit Rx8025t(TwoWire& wire = Wire) : _wire(wire) {}

  // Initialise la puce ; réinitialise ses registres si le flag VLF (perte
  // d'alimentation) est levé (comme upstream). Retourne false si absente sur
  // le bus (pas de réponse I2C).
  bool begin();

  // Heure courante (UTC), format C standard <time.h>.
  void getTime(tm* timeptr);
  void setTime(const tm* timeptr);

  bool isRunning();
  void setRunning(bool running);

  // VLF (Voltage Low Flag) : levé si l'alimentation a été coupée assez
  // longtemps pour perdre l'heure — à vérifier avant de faire confiance à
  // getTime() pour synchroniser une autre horloge (cf. hal/rtc/ExternalRtc.h).
  bool getVLF();
  void clearVLF();

private:
  enum RegAddr : uint8_t {
    REG_SEC = 0x00,
    REG_MIN = 0x01,
    REG_HOUR = 0x02,
    REG_WDAY = 0x03,
    REG_MDAY = 0x04,
    REG_MON = 0x05,
    REG_YEAR = 0x06,
    REG_FLAG = 0x0e,
    REG_CTRL = 0x0f,
  };

  uint8_t readReg(RegAddr addr);
  void writeReg(RegAddr addr, uint8_t val);

  TwoWire& _wire;
};

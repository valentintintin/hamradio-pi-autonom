#include "Rx8025t.h"

namespace {
constexpr uint8_t bcd2bin(uint8_t val) {
  return val - 6 * (val >> 4);
}
constexpr uint8_t bin2bcd(uint8_t val) {
  return val + 6 * (val / 10);
}
}  // namespace

uint8_t Rx8025t::readReg(RegAddr addr) {
  _wire.beginTransmission(ADDRESS);
  _wire.write(addr);
  _wire.endTransmission();

  _wire.requestFrom(ADDRESS, (uint8_t)1);
  return _wire.read();
}

void Rx8025t::writeReg(RegAddr addr, uint8_t val) {
  _wire.beginTransmission(ADDRESS);
  _wire.write(addr);
  _wire.write(val);
  _wire.endTransmission();
}

bool Rx8025t::begin() {
  _wire.beginTransmission(ADDRESS);
  _wire.write(REG_FLAG);
  if (_wire.endTransmission() != 0) {
    return false;
  }

  _wire.requestFrom(ADDRESS, (uint8_t)1);
  uint8_t flag = _wire.read();
  _wire.endTransmission();

  if (flag & 0x02) {
    // VLF levé : réinitialise tous les registres (comme upstream) — l'heure
    // sera de toute façon écrasée par la synchro au boot si une horloge de
    // référence est disponible (cf. hal/rtc/ExternalRtc.h).
    static const uint8_t init_regs[] = {
      REG_SEC,
      0x00,  // SEC
      0x00,  // MIN
      0x00,  // HOUR
      0x40,  // WEEK
      0x01,  // DAY
      0x01,  // MONTH
      0x00,  // YEAR
      0x00, 0x00, 0x00, 0x00,  // RAM, AL_MIN, AL_HOUR, AL_WK_D
      0x00, 0x00, 0x00,        // TIM0, TIM1, EXT
      0x00,                    // FLAG
      0x40,                    // CTRL
    };
    _wire.beginTransmission(ADDRESS);
    for (uint8_t i = 0; i < sizeof(init_regs); ++i) {
      _wire.write(init_regs[i]);
    }
    _wire.endTransmission();
  }

  return true;
}

void Rx8025t::getTime(tm* timeptr) {
  _wire.beginTransmission(ADDRESS);
  _wire.write(REG_SEC);
  _wire.endTransmission();

  _wire.requestFrom(ADDRESS, (uint8_t)7);
  timeptr->tm_sec = bcd2bin(_wire.read() & 0x7f);
  timeptr->tm_min = bcd2bin(_wire.read() & 0x7f);
  timeptr->tm_hour = bcd2bin(_wire.read() & 0x3f);
  timeptr->tm_wday = __builtin_ctz(_wire.read());
  timeptr->tm_mday = bcd2bin(_wire.read() & 0x3f);
  timeptr->tm_mon = bcd2bin(_wire.read() & 0x1f) - 1;
  timeptr->tm_year = bcd2bin(_wire.read()) + 100;
}

void Rx8025t::setTime(const tm* t) {
  const uint8_t write_buf[] = {
    REG_SEC,
    bin2bcd(t->tm_sec),
    bin2bcd(t->tm_min),
    bin2bcd(t->tm_hour),
    (uint8_t)(1U << t->tm_wday),
    bin2bcd(t->tm_mday),
    bin2bcd(t->tm_mon + 1),
    bin2bcd(t->tm_year - 100),
  };

  _wire.beginTransmission(ADDRESS);
  _wire.write(write_buf, sizeof(write_buf));
  _wire.endTransmission();
}

bool Rx8025t::isRunning() {
  return (readReg(REG_CTRL) & 0x01) == 0;
}

void Rx8025t::setRunning(bool running) {
  uint8_t mask = 0x01;
  uint8_t en = !running ? mask : 0;
  uint8_t val = readReg(REG_CTRL);
  if ((val & mask) != en) {
    writeReg(REG_CTRL, (uint8_t)((val & ~mask) | en));
  }
}

bool Rx8025t::getVLF() {
  return (readReg(REG_FLAG) & 0x02) != 0;
}

void Rx8025t::clearVLF() {
  uint8_t val = readReg(REG_FLAG);
  if (val & 0x02) {
    writeReg(REG_FLAG, (uint8_t)(val & ~0x02));
  }
}

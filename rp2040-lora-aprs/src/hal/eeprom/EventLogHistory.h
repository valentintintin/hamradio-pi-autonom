#pragma once

#include "M24M01Hal.h"
#include "EepromDumpHeader.h"
#include "core/Log.h"
#include <stdint.h>

#include "target.h"

// Plan mémoire EEPROM (128KB) : [0,1024) Settings · [1024,114688) TelemetryHistory
// · [114688=EVENT_LOG_ADDR, 131072) EventLogHistory. Les tailles réservées
// doivent rester cohérentes avec TELEMETRY_HISTORY_RESERVED_TAIL_BYTES.

#define EVENT_LOG_MAGIC            0x45564C33  // "EVL3"
#define EVENT_LOG_VERSION          3
#define EVENT_LOG_RESERVED_BYTES   (16 * 1024)
#define EVENT_LOG_ADDR             (M24M01_SIZE_BYTES - EVENT_LOG_RESERVED_BYTES)

// Ne pas renuméroter : les records déjà en EEPROM référencent ces valeurs.
enum EventCode : uint16_t {
  EVENT_NONE = 0,
  EVENT_WATCHDOG_REBOOT,           // data0 = nb de reboots watchdog consécutifs
  EVENT_WATCHDOG_TOO_MANY_REBOOTS, // data0 = nb de reboots watchdog consécutifs (chien désarmé pour ce cycle)
  EVENT_MPPT_SHUTDOWN_ALERT,       // data0 = source (0=GPIO, 1=I2C ALERT)
  EVENT_LOW_VOLTAGE_CUTOFF,        // data0 = numéro de relais (1..RELAY_COUNT), data1 = tension mV
  EVENT_LOW_VOLTAGE_RESTORE,       // data0 = numéro de relais (1..RELAY_COUNT), data1 = tension mV
  EVENT_RELAY_MANUAL_SET,          // data0 = numéro de relais (1..RELAY_COUNT), data1 = 1 (on) / 0 (off)
  EVENT_RELAY_MANUAL_CLEARED,      // data0 = numéro de relais (1..RELAY_COUNT) — "relay N auto"
};

inline const char* eventCodeName(uint16_t code) {
  switch (code) {
    case EVENT_WATCHDOG_REBOOT:           return "WDT_REBOOT";
    case EVENT_WATCHDOG_TOO_MANY_REBOOTS: return "WDT_TOO_MANY_REBOOTS";
    case EVENT_MPPT_SHUTDOWN_ALERT:       return "MPPT_SHUTDOWN";
    case EVENT_LOW_VOLTAGE_CUTOFF:        return "LOW_VOLTAGE_CUTOFF";
    case EVENT_LOW_VOLTAGE_RESTORE:       return "LOW_VOLTAGE_RESTORE";
    case EVENT_RELAY_MANUAL_SET:          return "RELAY_MANUAL_SET";
    case EVENT_RELAY_MANUAL_CLEARED:      return "RELAY_MANUAL_CLEARED";
    default:                              return "?";
  }
}

struct __attribute__((packed)) EventLogRecord {
  uint32_t timestamp;
  uint16_t code;   // EventCode
  int32_t data0;
  int32_t data1;
  int32_t data2;
  int32_t data3;
  int32_t data4;
};

struct __attribute__((packed)) EventLogHeader {
  uint32_t magic;
  uint16_t version;
  uint16_t write_index;
  uint16_t count;
  uint16_t max_records;
};

class EventLogHistory {
public:
  EventLogHistory(M24M01Hal& eeprom)
    : _eeprom(&eeprom), _initialized(false), _enabled(true), _max_records(0), _write_index(0), _count(0)
  {
  }

  bool begin() {
    if (!_eeprom->isInitialized()) {
      return false;
    }

    uint32_t data_start = EVENT_LOG_ADDR + sizeof(EventLogHeader);
    uint32_t available = M24M01_SIZE_BYTES - data_start;
    _max_records = available / sizeof(EventLogRecord);
    if (_max_records == 0) {
      return false;
    }

    EventLogHeader hdr;
    if (!_eeprom->read(EVENT_LOG_ADDR, (uint8_t*)&hdr, sizeof(hdr))) {
      return false;
    }

    if (hdr.magic == EVENT_LOG_MAGIC && hdr.version == EVENT_LOG_VERSION
        && hdr.max_records == _max_records) {
      _write_index = hdr.write_index % _max_records;
      _count = hdr.count > _max_records ? _max_records : hdr.count;
    } else {
      _write_index = 0;
      _count = 0;
      if (!saveHeader()) {
        return false;
      }
      LOG_I("EEPROM", "Log événements initialisé, %d slots", _max_records);
    }

    _initialized = true;
    LOG_I("EEPROM", "Log événements: %d/%d records", _count, _max_records);
    return true;
  }

  void setEnabled(bool enabled) { _enabled = enabled; }
  bool isEnabled() const { return _enabled; }

  bool log(uint16_t code, int32_t data0 = 0, int32_t data1 = 0,
           int32_t data2 = 0, int32_t data3 = 0, int32_t data4 = 0) {
    if (!_initialized || !_enabled) {
      return false;
    }

    EventLogRecord rec{};
    rec.timestamp = rtc_clock.getCurrentTime();
    rec.code = code;
    rec.data0 = data0;
    rec.data1 = data1;
    rec.data2 = data2;
    rec.data3 = data3;
    rec.data4 = data4;

    uint32_t addr = recordAddr(_write_index);
    if (!_eeprom->write(addr, (const uint8_t*)&rec, sizeof(rec))) {
      return false;
    }

    _write_index = (_write_index + 1) % _max_records;
    if (_count < _max_records) {
      _count++;
    }

    return saveHeader();
  }

  // index 0 = record le plus ancien
  bool readRecord(uint16_t index, EventLogRecord& rec) const {
    if (!_initialized || index >= _count) {
      return false;
    }

    uint16_t actual = (_write_index + _max_records - _count + index) % _max_records;
    return _eeprom->read(recordAddr(actual), (uint8_t*)&rec, sizeof(rec));
  }

  void dumpBinary(Print& out, uint16_t last_n = 0) const {
    uint16_t n = (last_n > 0 && last_n < _count) ? last_n : _count;
    uint16_t start = _count - n;

    EepromDumpHeader hdr{ EVENT_LOG_MAGIC, (uint16_t)sizeof(EventLogRecord), n };
    out.write((const uint8_t*)&hdr, sizeof(hdr));

    EventLogRecord rec{};
    for (uint16_t i = start; i < _count; i++) {
      if (readRecord(i, rec)) {
        out.write((const uint8_t*)&rec, sizeof(rec));
      }
    }
  }

  bool clear() {
    _write_index = 0;
    _count = 0;
    return saveHeader();
  }

  uint16_t getCount() const { return _count; }
  uint16_t getMaxRecords() const { return _max_records; }
  bool isInitialized() const { return _initialized; }

private:
  M24M01Hal* _eeprom;
  bool _initialized;
  bool _enabled;
  uint16_t _max_records;
  uint16_t _write_index;
  uint16_t _count;

  uint32_t recordAddr(uint16_t index) const {
    return EVENT_LOG_ADDR + sizeof(EventLogHeader)
           + (uint32_t)index * sizeof(EventLogRecord);
  }

  bool saveHeader() {
    EventLogHeader hdr;
    hdr.magic = EVENT_LOG_MAGIC;
    hdr.version = EVENT_LOG_VERSION;
    hdr.write_index = _write_index;
    hdr.count = _count;
    hdr.max_records = _max_records;
    return _eeprom->write(EVENT_LOG_ADDR, (const uint8_t*)&hdr, sizeof(hdr));
  }
};

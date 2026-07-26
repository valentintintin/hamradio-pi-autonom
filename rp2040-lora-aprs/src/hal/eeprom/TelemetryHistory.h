#pragma once

#include "M24M01Hal.h"
#include "hal/Telemetry.h"
#include "EepromDumpHeader.h"
#include "core/Log.h"
#include <stdint.h>

#include "target.h"

// ============================================================================
// TelemetryHistory — Ring buffer sur EEPROM M24M01
//
// Stocke un historique compact de télémétrie pour analyse post-mortem.
// Avec 21 bytes/record et ~111KB disponibles (16KB réservés en fin
// d'EEPROM pour EventLogHistory, cf. plus bas) : ~5411 records.
// À 5min/record : ~18 jours d'historique.
//
// Plan mémoire EEPROM (128KB, cf. M24M01_SIZE_BYTES) — cf. hal/eeprom/EventLogHistory.h
// pour le détail complet et la vérification de non-chevauchement :
//   [0, 1024) Settings · [1024, 114688) TelemetryHistory · [114688, 131072) EventLogHistory
// ============================================================================

#define TELEMETRY_HISTORY_MAGIC     0x54454C48  // "TELH"
#define TELEMETRY_HISTORY_VERSION   2  // v2 : TelemetryRecord.battery/solar en CompactEnergyData (21 bytes, pas 29)

// Adresse EEPROM (après la zone settings — 1KB de marge)
#define TELEMETRY_HISTORY_ADDR      1024

// Réservés en fin d'EEPROM pour EventLogHistory (cf. hal/eeprom/EventLogHistory.h,
// EVENT_LOG_RESERVED_BYTES — les deux constantes doivent rester cohérentes).
#define TELEMETRY_HISTORY_RESERVED_TAIL_BYTES (16 * 1024)

// Type compact dédié au stockage (pas EnergyData de Telemetry.h, qui est en
// float — 8 bytes/champ au lieu de 4, et ferait passer TelemetryRecord de
// 21 à 29 bytes en plus d'un mismatch de type avec le "%d" de dump()).
struct __attribute__((packed)) CompactEnergyData {
  int16_t voltage_mv;
  int16_t current_ma;
};

// Record compact (21 bytes)
struct __attribute__((packed)) TelemetryRecord {
  uint32_t timestamp;
  CompactEnergyData battery;
  CompactEnergyData solar;
  int16_t  temperature_inside_c10;  // température * 10
  uint8_t  humidity_inside;         // 0-100
  int16_t  temperature_outside_c10;  // température * 10
  uint32_t uptime_s;
};

// Header du ring buffer (12 bytes)
struct __attribute__((packed)) TelemetryHistoryHeader {
  uint32_t magic;
  uint16_t version;
  uint16_t write_index;      // prochain slot d'écriture (0..max_records-1)
  uint16_t count;            // nombre d'enregistrements valides
  uint16_t max_records;      // capacité totale
};

class TelemetryHistory {
public:
  TelemetryHistory(M24M01Hal& eeprom)
    : _eeprom(&eeprom), _initialized(false), _max_records(0), _write_index(0), _count(0)
  {
  }

  bool begin() {
    if (!_eeprom->isInitialized()) {
      return false;
    }

    // Calculer la capacité disponible
    uint32_t data_start = TELEMETRY_HISTORY_ADDR + sizeof(TelemetryHistoryHeader);
    uint32_t available = M24M01_SIZE_BYTES - TELEMETRY_HISTORY_RESERVED_TAIL_BYTES - data_start;
    _max_records = available / sizeof(TelemetryRecord);
    if (_max_records == 0) {
      return false;
    }

    // Lire le header existant
    TelemetryHistoryHeader hdr;
    if (!_eeprom->read(TELEMETRY_HISTORY_ADDR, (uint8_t*)&hdr, sizeof(hdr))) {
      return false;
    }

    if (hdr.magic == TELEMETRY_HISTORY_MAGIC && hdr.version == TELEMETRY_HISTORY_VERSION
        && hdr.max_records == _max_records) {
      // Header valide, reprendre là où on en était
      _write_index = hdr.write_index % _max_records;
      _count = hdr.count > _max_records ? _max_records : hdr.count;
    } else {
      // Initialiser un nouveau ring buffer
      _write_index = 0;
      _count = 0;
      if (!saveHeader()) {
        return false;
      }
      LOG_I("EEPROM", "Historique initialisé, %d slots", _max_records);
    }

    _initialized = true;
    LOG_I("EEPROM", "Historique: %d/%d records", _count, _max_records);
    return true;
  }

  // Enregistrer un snapshot de télémétrie
  bool record(const TelemetryData& t) {
    if (!_initialized) {
      return false;
    }

    TelemetryRecord rec;
    rec.timestamp = rtc_clock.getCurrentTime();
    rec.battery.voltage_mv = (int16_t)t.battery_mppt.voltage_mv;
    rec.battery.current_ma = (int16_t)t.battery_mppt.current_ma;
    rec.solar.voltage_mv = (int16_t)t.solar_mppt.voltage_mv;
    rec.solar.current_ma = (int16_t)t.solar_mppt.current_ma;
    rec.temperature_inside_c10 = (int16_t)(t.weather_inside.base.temperature_c * 10.0f);
    rec.humidity_inside = (uint8_t)t.weather_inside.base.humidity;
    rec.temperature_outside_c10 = (int16_t)(t.weather_outside.base.temperature_c * 10.0f);
    rec.uptime_s = millis();

    uint32_t addr = recordAddr(_write_index);
    if (!_eeprom->write(addr, (const uint8_t*)&rec, sizeof(rec))) {
      LOG_W("EEPROM", "Erreur écriture record %d", _write_index);
      return false;
    }

    _write_index = (_write_index + 1) % _max_records;
    if (_count < _max_records) {
      _count++;
    }

    return saveHeader();
  }

  // Lire un record par index (0 = plus ancien)
  bool readRecord(uint16_t index, TelemetryRecord& rec) const {
    if (!_initialized || index >= _count) {
      return false;
    }

    // Le plus ancien est à (write_index - count) mod max
    uint16_t actual = (_write_index + _max_records - _count + index) % _max_records;
    return _eeprom->read(recordAddr(actual), (uint8_t*)&rec, sizeof(rec));
  }

  // Dump vers Print (serial ou APRS response buffer). includeHeader=false
  // omet la bannière et l'en-tête CSV (cf. CommandHandler::cmdHistory, cas
  // "history 1" en distant APRS/mesh où le tampon de réponse est minuscule).
  void dump(Print& out, uint16_t last_n = 0, bool includeHeader = true) const {
    uint16_t n = (last_n > 0 && last_n < _count) ? last_n : _count;
    uint16_t start = _count - n;

    if (includeHeader) {
      out.printf("--- Historique: %d/%d records ---\n", _count, _max_records);
      out.println(F("date,bat_mV,bat_mA,sol_mV,sol_mA,temp_in,hum_in,temp_out,uptime"));
    }

    TelemetryRecord rec{};
    for (uint16_t i = start; i < _count; i++) {
      if (readRecord(i, rec)) {
        out.printf("%lu,%d,%d,%d,%d,%.1f,%d,%.1f,%d\n",
          rec.timestamp,
          rec.battery.voltage_mv, rec.battery.current_ma,
          rec.solar.voltage_mv, rec.solar.current_ma,
          rec.temperature_inside_c10 / 10.0f, rec.humidity_inside,
          rec.temperature_outside_c10 / 10.0f,
          rec.uptime_s);
      }
    }
  }

  // Dump binaire brut (EepromDumpHeader + records packed) — bien plus rapide
  // à produire (pas de formatage) et à transférer que dump() ; utilisé par
  // le CLI "history dump" (cf. CommandHandler), destiné à un outil côté PC
  // qui reparse ces records directement plutôt qu'à une lecture humaine.
  void dumpBinary(Print& out, uint16_t last_n = 0) const {
    uint16_t n = (last_n > 0 && last_n < _count) ? last_n : _count;
    uint16_t start = _count - n;

    EepromDumpHeader hdr{ TELEMETRY_HISTORY_MAGIC, (uint16_t)sizeof(TelemetryRecord), n };
    out.write((const uint8_t*)&hdr, sizeof(hdr));

    TelemetryRecord rec{};
    for (uint16_t i = start; i < _count; i++) {
      if (readRecord(i, rec)) {
        out.write((const uint8_t*)&rec, sizeof(rec));
      }
    }
  }

  // Reset
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
  uint16_t _max_records;
  uint16_t _write_index;
  uint16_t _count;

  uint32_t recordAddr(uint16_t index) const {
    return TELEMETRY_HISTORY_ADDR + sizeof(TelemetryHistoryHeader)
           + (uint32_t)index * sizeof(TelemetryRecord);
  }

  bool saveHeader() {
    TelemetryHistoryHeader hdr;
    hdr.magic = TELEMETRY_HISTORY_MAGIC;
    hdr.version = TELEMETRY_HISTORY_VERSION;
    hdr.write_index = _write_index;
    hdr.count = _count;
    hdr.max_records = _max_records;
    return _eeprom->write(TELEMETRY_HISTORY_ADDR, (const uint8_t*)&hdr, sizeof(hdr));
  }
};

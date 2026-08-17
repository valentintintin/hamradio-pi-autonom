#include "hal/eeprom/M24M01Hal.h"
#include <Arduino.h>

#include "SimPaths.h"
#include "SimSettingsJson.h"
#include "hal/eeprom/TelemetryHistory.h"
#include "hal/eeprom/EventLogHistory.h"
#include <ArduinoJson.h>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <string>

// Le plan mémoire (adresses/tailles) est dupliqué depuis hal/eeprom/TelemetryHistory.h
// et EventLogHistory.h — à garder synchronisé si ce plan mémoire change.
namespace {

enum class Zone { Settings, TelemetryHeader, TelemetryRecord, EventLogHeader, EventLogRecord };

constexpr uint32_t kTelemetryAddr = 1024;
constexpr uint32_t kTelemetryHdrSize = 12;
constexpr uint32_t kTelemetryRecSize = 21;
constexpr uint32_t kEventLogAddr = 114688;
constexpr uint32_t kEventLogHdrSize = 12;
constexpr uint32_t kEventLogRecSize = 26;

std::string eepromDir() {
  return simDataDir() + "/eeprom";
}

Zone locateZone(uint32_t address, std::string& path) {
  std::string dir = eepromDir();
  char buf[48];

  if (address < kTelemetryAddr) {
    path = dir + "/settings.json";
    return Zone::Settings;
  }
  if (address < kEventLogAddr) {
    if (address == kTelemetryAddr) {
      path = dir + "/telemetry_history/header.json";
      return Zone::TelemetryHeader;
    }
    uint32_t index = (address - kTelemetryAddr - kTelemetryHdrSize) / kTelemetryRecSize;
    snprintf(buf, sizeof(buf), "/telemetry_history/%05u.json", (unsigned)index);
    path = dir + buf;
    return Zone::TelemetryRecord;
  }
  if (address == kEventLogAddr) {
    path = dir + "/event_log/header.json";
    return Zone::EventLogHeader;
  }
  uint32_t index = (address - kEventLogAddr - kEventLogHdrSize) / kEventLogRecSize;
  snprintf(buf, sizeof(buf), "/event_log/%05u.json", (unsigned)index);
  path = dir + buf;
  return Zone::EventLogRecord;
}

template <typename Header>
std::string headerToJson(const Header& h) {
  JsonDocument doc;
  doc["magic"] = h.magic;
  doc["version"] = h.version;
  doc["write_index"] = h.write_index;
  doc["count"] = h.count;
  doc["max_records"] = h.max_records;
  std::string out;
  serializeJsonPretty(doc, out);
  return out;
}

template <typename Header>
bool headerFromJson(const std::string& json, Header& h) {
  JsonDocument doc;
  if (deserializeJson(doc, json) != DeserializationError::Ok) {
    return false;
  }
  h.magic = doc["magic"] | 0;
  h.version = doc["version"] | 0;
  h.write_index = doc["write_index"] | 0;
  h.count = doc["count"] | 0;
  h.max_records = doc["max_records"] | 0;
  return true;
}

std::string telemetryRecordToJson(const TelemetryRecord& r) {
  JsonDocument doc;
  doc["timestamp"] = r.timestamp;
  doc["battery_voltage_mv"] = r.battery.voltage_mv;
  doc["battery_current_ma"] = r.battery.current_ma;
  doc["solar_voltage_mv"] = r.solar.voltage_mv;
  doc["solar_current_ma"] = r.solar.current_ma;
  doc["temperature_inside_c10"] = r.temperature_inside_c10;
  doc["humidity_inside"] = r.humidity_inside;
  doc["temperature_outside_c10"] = r.temperature_outside_c10;
  doc["uptime_s"] = r.uptime_s;
  std::string out;
  serializeJsonPretty(doc, out);
  return out;
}

bool telemetryRecordFromJson(const std::string& json, TelemetryRecord& r) {
  JsonDocument doc;
  if (deserializeJson(doc, json) != DeserializationError::Ok) {
    return false;
  }
  r.timestamp = doc["timestamp"] | 0;
  r.battery.voltage_mv = doc["battery_voltage_mv"] | 0;
  r.battery.current_ma = doc["battery_current_ma"] | 0;
  r.solar.voltage_mv = doc["solar_voltage_mv"] | 0;
  r.solar.current_ma = doc["solar_current_ma"] | 0;
  r.temperature_inside_c10 = doc["temperature_inside_c10"] | 0;
  r.humidity_inside = doc["humidity_inside"] | 0;
  r.temperature_outside_c10 = doc["temperature_outside_c10"] | 0;
  r.uptime_s = doc["uptime_s"] | 0;
  return true;
}

std::string eventLogRecordToJson(const EventLogRecord& r) {
  JsonDocument doc;
  doc["timestamp"] = r.timestamp;
  doc["code"] = r.code;
  doc["code_name"] = eventCodeName(r.code);
  doc["data0"] = r.data0;
  doc["data1"] = r.data1;
  doc["data2"] = r.data2;
  doc["data3"] = r.data3;
  doc["data4"] = r.data4;
  std::string out;
  serializeJsonPretty(doc, out);
  return out;
}

bool eventLogRecordFromJson(const std::string& json, EventLogRecord& r) {
  JsonDocument doc;
  if (deserializeJson(doc, json) != DeserializationError::Ok) {
    return false;
  }
  r.timestamp = doc["timestamp"] | 0;
  r.code = doc["code"] | 0;
  r.data0 = doc["data0"] | 0;
  r.data1 = doc["data1"] | 0;
  r.data2 = doc["data2"] | 0;
  r.data3 = doc["data3"] | 0;
  r.data4 = doc["data4"] | 0;
  return true;
}

bool readFileToString(const std::string& path, std::string& out) {
  FILE* f = fopen(path.c_str(), "rb");
  if (!f) {
    return false;
  }
  fseek(f, 0, SEEK_END);
  long sz = ftell(f);
  fseek(f, 0, SEEK_SET);
  out.resize(sz > 0 ? (size_t)sz : 0);
  if (sz > 0) {
    size_t n = fread(&out[0], 1, out.size(), f);
    out.resize(n);
  }
  fclose(f);
  return true;
}

bool writeStringToFile(const std::string& path, const std::string& content) {
  FILE* f = fopen(path.c_str(), "wb");
  if (!f) {
    return false;
  }
  size_t n = fwrite(content.data(), 1, content.size(), f);
  fclose(f);
  return n == content.size();
}

std::mutex g_eeprom_mutex;

}  // namespace

bool M24M01Hal::begin() {
  std::lock_guard<std::mutex> lock(g_eeprom_mutex);
  simMkdirs(eepromDir());
  simMkdirs(eepromDir() + "/telemetry_history");
  simMkdirs(eepromDir() + "/event_log");
  _initialized = true;
  return true;
}

bool M24M01Hal::read(uint32_t address, uint8_t* data, size_t len) {
  if (!_initialized || address + len > M24M01_SIZE_BYTES) {
    return false;
  }
  std::lock_guard<std::mutex> lock(g_eeprom_mutex);

  std::string path;
  Zone zone = locateZone(address, path);

  std::string json;
  if (!readFileToString(path, json)) {
    memset(data, 0xFF, len);  // 0xFF = état "vierge" d'une EEPROM jamais écrite, comme le vrai chip
    return true;
  }

  bool ok;
  switch (zone) {
    case Zone::Settings:
      ok = settingsFromJson(json, *reinterpret_cast<Settings*>(data));
      break;
    case Zone::TelemetryHeader:
      ok = headerFromJson(json, *reinterpret_cast<TelemetryHistoryHeader*>(data));
      break;
    case Zone::TelemetryRecord:
      ok = telemetryRecordFromJson(json, *reinterpret_cast<TelemetryRecord*>(data));
      break;
    case Zone::EventLogHeader:
      ok = headerFromJson(json, *reinterpret_cast<EventLogHeader*>(data));
      break;
    default:
      ok = eventLogRecordFromJson(json, *reinterpret_cast<EventLogRecord*>(data));
      break;
  }
  if (!ok) {
    memset(data, 0xFF, len);
  }
  return true;
}

bool M24M01Hal::write(uint32_t address, const uint8_t* data, size_t len) {
  if (!_initialized || address + len > M24M01_SIZE_BYTES) {
    return false;
  }
  std::lock_guard<std::mutex> lock(g_eeprom_mutex);

  std::string path;
  Zone zone = locateZone(address, path);

  std::string json;
  switch (zone) {
    case Zone::Settings:
      json = settingsToJson(*reinterpret_cast<const Settings*>(data));
      break;
    case Zone::TelemetryHeader:
      json = headerToJson(*reinterpret_cast<const TelemetryHistoryHeader*>(data));
      break;
    case Zone::TelemetryRecord:
      json = telemetryRecordToJson(*reinterpret_cast<const TelemetryRecord*>(data));
      break;
    case Zone::EventLogHeader:
      json = headerToJson(*reinterpret_cast<const EventLogHeader*>(data));
      break;
    default:
      json = eventLogRecordToJson(*reinterpret_cast<const EventLogRecord*>(data));
      break;
  }
  return writeStringToFile(path, json);
}

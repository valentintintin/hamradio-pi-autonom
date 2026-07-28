#include "config/SettingsCodec.h"
#include "SimSettingsJson.h"
#include <LittleFS.h>
#include <string>

#define SETTINGS_FILE "/settings.json"

namespace {
bool isValid(const Settings& s) {
  return s.magic == SETTINGS_MAGIC && s.version == SETTINGS_VERSION;
}
}  // namespace

bool settingsLoadFromLittleFS(Settings& s) {
  File f = LittleFS.open(SETTINGS_FILE, "r");
  if (!f) {
    return false;
  }
  std::string json;
  json.resize(f.size());
  if (!json.empty()) {
    f.read((uint8_t*)&json[0], json.size());
  }
  f.close();
  return settingsFromJson(json, s) && isValid(s);
}

bool settingsSaveToLittleFS(const Settings& s) {
  File f = LittleFS.open(SETTINGS_FILE, "w");
  if (!f) {
    return false;
  }
  std::string json = settingsToJson(s);
  size_t written = f.write((const uint8_t*)json.data(), json.size());
  f.close();
  return written == json.size();
}

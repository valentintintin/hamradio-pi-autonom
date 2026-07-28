#include "SimSettingsJson.h"
#include "config/SettingsRegistry.h"
#include <ArduinoJson.h>

std::string settingsToJson(const Settings& s) {
  JsonDocument doc;

  SettingsRegistry reg;
  reg.init(const_cast<Settings&>(s));

  char buf[80];
  for (int i = 0; i < reg.count(); i++) {
    const SettingEntry* e = reg.entryAt(i);
    if (!e) {
      continue;
    }
    reg.get(e->key, buf, sizeof(buf));
    doc[e->key] = buf;
  }

  std::string out;
  serializeJsonPretty(doc, out);
  return out;
}

bool settingsFromJson(const std::string& json, Settings& s) {
  JsonDocument doc;
  if (deserializeJson(doc, json) != DeserializationError::Ok) {
    return false;
  }

  s = getDefaultSettings();
  SettingsRegistry reg;
  reg.init(s);

  for (JsonPairConst kv : doc.as<JsonObjectConst>()) {
    const char* value = kv.value().as<const char*>();
    if (value) {
      reg.set(kv.key().c_str(), value);
    }
  }
  return true;
}

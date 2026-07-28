#include "SettingsCodec.h"
#include <LittleFS.h>

#define SETTINGS_FILE "/settings.bin"

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
  if (f.size() != sizeof(Settings)) {
    f.close();
    return false;
  }
  size_t read = f.read((uint8_t*)&s, sizeof(Settings));
  f.close();
  return read == sizeof(Settings) && isValid(s);
}

bool settingsSaveToLittleFS(const Settings& s) {
  File f = LittleFS.open(SETTINGS_FILE, "w");
  if (!f) {
    return false;
  }
  size_t written = f.write((const uint8_t*)&s, sizeof(Settings));
  f.close();
  return written == sizeof(Settings);
}

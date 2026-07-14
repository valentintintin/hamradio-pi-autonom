#pragma once

#include "Settings.h"
#include "Log.h"
#include "hal/EepromHal.h"
#include <LittleFS.h>

// ============================================================================
// SettingsManager — charge/sauvegarde de la config
//
// Priorité : LittleFS → EEPROM → défauts
// Format : struct binaire brute (pas de JSON, pas d'allocations)
// ============================================================================

#define SETTINGS_FILE       "/settings.bin"
#define SETTINGS_EEPROM_ADDR 0  // début EEPROM pour la config

class SettingsManager {
public:
  SettingsManager(EepromHal* eeprom = nullptr)
    : _eeprom(eeprom) {}

  // Charger la config (LittleFS > EEPROM > défauts)
  Settings load() {
    Settings s;

    // Essayer LittleFS
    if (loadFromLittleFS(s)) {
      LOG_I("CONFIG", "Chargée depuis LittleFS");
      return s;
    }

    // Essayer EEPROM
    if (_eeprom && _eeprom->isInitialized() && loadFromEeprom(s)) {
      LOG_I("CONFIG", "Chargée depuis EEPROM (copie vers LittleFS)");
      saveToLittleFS(s);
      return s;
    }

    // Défauts
    LOG_W("CONFIG", "Aucune config trouvée, défauts chargés");
    s = getDefaultSettings();
    save(s);
    return s;
  }

  // Sauvegarder sur les deux supports
  bool save(const Settings& s) {
    bool ok = saveToLittleFS(s);
    if (_eeprom && _eeprom->isInitialized()) {
      ok &= saveToEeprom(s);
    }
    return ok;
  }

  void setEeprom(EepromHal* eeprom) { _eeprom = eeprom; }

private:
  EepromHal* _eeprom;

  bool isValid(const Settings& s) {
    return s.magic == SETTINGS_MAGIC && s.version == SETTINGS_VERSION;
  }

  bool loadFromLittleFS(Settings& s) {
    File f = LittleFS.open(SETTINGS_FILE, "r");
    if (!f) {
      return false;
    }
    if (f.size() != sizeof(Settings)) { f.close(); return false; }
    size_t read = f.read((uint8_t*)&s, sizeof(Settings));
    f.close();
    return read == sizeof(Settings) && isValid(s);
  }

  bool saveToLittleFS(const Settings& s) {
    File f = LittleFS.open(SETTINGS_FILE, "w");
    if (!f) {
      return false;
    }
    size_t written = f.write((const uint8_t*)&s, sizeof(Settings));
    f.close();
    return written == sizeof(Settings);
  }

  bool loadFromEeprom(Settings& s) {
    if (!_eeprom->read(SETTINGS_EEPROM_ADDR, (uint8_t*)&s, sizeof(Settings)))
      return false;
    return isValid(s);
  }

  bool saveToEeprom(const Settings& s) {
    return _eeprom->write(SETTINGS_EEPROM_ADDR, (const uint8_t*)&s, sizeof(Settings));
  }
};

#pragma once

#include "Settings.h"
#include "SettingsCodec.h"
#include "core/Log.h"
#include "hal/eeprom/M24M01Hal.h"

// Priorité de chargement : LittleFS → EEPROM → défauts.
#define SETTINGS_EEPROM_ADDR 0

class SettingsManager {
public:
  SettingsManager(M24M01Hal* eeprom = nullptr)
    : _eeprom(eeprom) {}

  Settings load() {
    Settings s{};

    if (settingsLoadFromLittleFS(s)) {
      LOG_I("CONFIG", "Chargée depuis LittleFS");
      if (_eeprom && _eeprom->isInitialized()) {
        saveToEeprom(s);
      }
      return s;
    }

    if (_eeprom && _eeprom->isInitialized() && loadFromEeprom(s)) {
      LOG_I("CONFIG", "Chargée depuis EEPROM (copie vers LittleFS)");
      settingsSaveToLittleFS(s);
      return s;
    }

    LOG_W("CONFIG", "Aucune config trouvée, défauts chargés");
    s = getDefaultSettings();
    save(s);
    return s;
  }

  bool save(const Settings& s) {
    bool ok = settingsSaveToLittleFS(s);
    if (_eeprom && _eeprom->isInitialized()) {
      ok &= saveToEeprom(s);
    }
    return ok;
  }

  void setEeprom(M24M01Hal* eeprom) { _eeprom = eeprom; }

private:
  M24M01Hal* _eeprom;

  bool isValid(const Settings& s) {
    return s.magic == SETTINGS_MAGIC && s.version == SETTINGS_VERSION;
  }

  bool loadFromEeprom(Settings& s) {
    if (!_eeprom->read(SETTINGS_EEPROM_ADDR, (uint8_t*)&s, sizeof(Settings)))
      return false;
    return isValid(s);
  }

  // Comparaison octet à octet plutôt qu'un hash : la lecture de vérification magic/version est déjà nécessaire,
  // donc même coût I/O sans risque de collision. Évite l'usure EEPROM si rien n'a changé.
  bool saveToEeprom(const Settings& s) {
    Settings current{};
    if (loadFromEeprom(current) && memcmp(&current, &s, sizeof(Settings)) == 0) {
      return true;
    }
    return _eeprom->write(SETTINGS_EEPROM_ADDR, (const uint8_t*)&s, sizeof(Settings));
  }
};

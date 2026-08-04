#pragma once

#include "Settings.h"
#include "SettingsCodec.h"
#include "core/Log.h"
#include "hal/eeprom/M24M01Hal.h"

// ============================================================================
// SettingsManager — charge/sauvegarde de la config
//
// Priorité : LittleFS → EEPROM → défauts
// Format : struct binaire brute (pas de JSON, pas d'allocations) — sauf en
// natif, où le fichier LittleFS est en JSON lisible/éditable ; cf.
// SettingsCodec.h pour les deux implémentations (une par environnement,
// sélectionnées par platformio.ini — la copie EEPROM suit le même principe
// côté hal/eeprom/M24M01Hal.cpp / native_sim/sim/M24M01HalSim.cpp).
// ============================================================================

#define SETTINGS_EEPROM_ADDR 0  // début EEPROM pour la config

class SettingsManager {
public:
  SettingsManager(M24M01Hal* eeprom = nullptr)
    : _eeprom(eeprom) {}

  // Charger la config (LittleFS > EEPROM > défauts)
  Settings load() {
    Settings s{};

    // Essayer LittleFS
    if (settingsLoadFromLittleFS(s)) {
      LOG_I("CONFIG", "Chargée depuis LittleFS");
      if (_eeprom && _eeprom->isInitialized()) {
        saveToEeprom(s);
      }
      return s;
    }

    // Essayer EEPROM
    if (_eeprom && _eeprom->isInitialized() && loadFromEeprom(s)) {
      LOG_I("CONFIG", "Chargée depuis EEPROM (copie vers LittleFS)");
      settingsSaveToLittleFS(s);
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

  // N'écrit que si le contenu diffère de la copie EEPROM actuelle — évite
  // l'usure de l'EEPROM (écriture à chaque boot alors que rien n'a changé).
  // Comparaison directe des octets bruts plutôt qu'un hash : on doit de
  // toute façon relire sizeof(Settings) depuis l'EEPROM pour vérifier
  // magic/version (cf. loadFromEeprom/isValid), donc même coût I/O qu'un
  // hash, sans risque de collision.
  bool saveToEeprom(const Settings& s) {
    Settings current{};
    if (loadFromEeprom(current) && memcmp(&current, &s, sizeof(Settings)) == 0) {
      return true;
    }
    return _eeprom->write(SETTINGS_EEPROM_ADDR, (const uint8_t*)&s, sizeof(Settings));
  }
};

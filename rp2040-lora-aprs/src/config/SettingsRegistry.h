#pragma once

#include "Settings.h"
#include <Arduino.h>

// ============================================================================
// SettingsRegistry — table clé/type/pointeur pour accès get/set par nom
//
// Inspiré de l'ancien System.h avec le tableau de SettingEntry.
// Permet d'accéder à n'importe quel champ par "aprs.callsign", "weather.wh65b_enabled" etc.
// ============================================================================

enum SettingType {
  ST_STRING,    // char[]
  ST_FLOAT,
  ST_INT8,
  ST_INT16,
  ST_UINT8,
  ST_UINT16,
  ST_UINT32,
  ST_BOOL,
  ST_ENUM8      // uint8_t réglé/affiché par nom plutôt que par valeur numérique (cf. EnumNameEntry)
};

// Table nom<->valeur pour un champ ST_ENUM8 (ex: "info"=3 pour system.log_level,
// "robot36"=0 pour sstv.mode). Une table par champ enum, réutilisable.
struct EnumNameEntry {
  const char* name;
  uint8_t value;
};

struct SettingEntry {
  const char* key;
  SettingType type;
  void* ptr;        // pointeur dans la struct Settings
  uint8_t maxLen;   // pour ST_STRING uniquement
  float min;        // borne min (ignorée pour ST_STRING, ST_BOOL, ST_ENUM8)
  float max;        // borne max
  bool hasRange;    // true si min/max sont définis
  const EnumNameEntry* enumTable;  // non-nul seulement pour ST_ENUM8
  uint8_t enumTableCount;
};

// ============================================================================
// Classe registre
// ============================================================================
class SettingsRegistry {
public:
  // Initialise les pointeurs vers la struct settings
  void init(Settings& settings);

  // Nombre d'entrées
  int count() const { return _count; }

  // Chercher une entrée par clé
  const SettingEntry* find(const char* key) const;

  // Lire une valeur en string (pour affichage CLI)
  bool get(const char* key, char* out, size_t outLen) const;

  // Écrire une valeur depuis un string (parsing + validation bornes)
  bool set(const char* key, const char* value);
  bool set(const char* key, const char* value, Print* out);

  // Lister toutes les clés (pour "list" ou "help")
  void listAll(Print& out) const;

private:
  SettingEntry _entries[96]{};  // assez large pour tous les champs
  int _count = 0;

  void add(const char* key, SettingType type, void* ptr, uint8_t maxLen = 0);
  void add(const char* key, SettingType type, void* ptr, float min, float max);
  void add(const char* key, void* ptr, const EnumNameEntry* table, uint8_t tableCount);

  // Valide une valeur numérique contre les bornes
  bool validate(const SettingEntry* e, float value, Print* out = nullptr) const;
};

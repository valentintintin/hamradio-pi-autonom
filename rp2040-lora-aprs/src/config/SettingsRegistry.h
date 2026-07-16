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
  ST_BOOL
};

struct SettingEntry {
  const char* key;
  SettingType type;
  void* ptr;        // pointeur dans la struct Settings
  uint8_t maxLen;   // pour ST_STRING uniquement
  float min;        // borne min (ignorée pour ST_STRING et ST_BOOL)
  float max;        // borne max
  bool hasRange;    // true si min/max sont définis
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
  SettingEntry _entries[64]{};  // assez large pour tous les champs
  int _count = 0;

  void add(const char* key, SettingType type, void* ptr, uint8_t maxLen = 0);
  void add(const char* key, SettingType type, void* ptr, float min, float max);

  // Valide une valeur numérique contre les bornes
  bool validate(const SettingEntry* e, float value, Print* out = nullptr) const;
};

#pragma once

#include "Settings.h"
#include <Arduino.h>

enum SettingType {
  ST_STRING,    // char[]
  ST_FLOAT,
  ST_INT8,
  ST_INT16,
  ST_UINT8,
  ST_UINT16,
  ST_UINT32,
  ST_BOOL,
  ST_ENUM8      // uint8_t réglé/affiché par nom plutôt que par valeur numérique
};

struct EnumNameEntry {
  const char* name;
  uint8_t value;
};

struct SettingEntry {
  const char* key;
  SettingType type;
  void* ptr;
  uint8_t maxLen;
  float min;
  float max;
  bool hasRange;
  const EnumNameEntry* enumTable;
  uint8_t enumTableCount;
};

class SettingsRegistry {
public:
  void init(Settings& settings);

  int count() const { return _count; }

  const SettingEntry* find(const char* key) const;

  bool get(const char* key, char* out, size_t outLen) const;

  bool set(const char* key, const char* value);
  bool set(const char* key, const char* value, Print* out);

  void listAll(Print& out) const;

  const SettingEntry* entryAt(int index) const {
    return (index >= 0 && index < _count) ? &_entries[index] : nullptr;
  }

private:
  SettingEntry _entries[128]{};
  int _count = 0;

  void add(const char* key, SettingType type, void* ptr, uint8_t maxLen = 0);
  void add(const char* key, SettingType type, void* ptr, float min, float max);
  void add(const char* key, void* ptr, const EnumNameEntry* table, uint8_t tableCount);

  bool validate(const SettingEntry* e, float value, Print* out = nullptr) const;
};

#pragma once

#include "Arduino.h"  // Stream
#include <cstdio>

// ============================================================================
// FS.h — shim natif du système de fichiers générique arduino-pico (fs::FS /
// File), dont LittleFS.h fournit l'implémentation concrète. Nécessaire pour
// lib/MeshCore/src/helpers/{IdentityStore,ClientACL,RegionMap,CommonCLI} qui
// manipulent un `FILESYSTEM*` (= fs::FS* sur la branche RP2040_PLATFORM,
// définie pour l'env natif) sans connaître LittleFS directement.
//
// File s'appuie sur un vrai fichier POSIX (FILE*) — cf. native/compat/
// littlefs_compat.cpp pour la résolution des chemins sous SIM_DATA_DIR.
// ============================================================================

class File : public Stream {
public:
  File() : _fp(nullptr) {}
  explicit File(FILE* fp) : _fp(fp) {}

  operator bool() const { return _fp != nullptr; }

  size_t write(uint8_t c) override { return _fp ? fwrite(&c, 1, 1, _fp) : 0; }
  size_t write(const uint8_t* buf, size_t len) override {
    return _fp ? fwrite(buf, 1, len, _fp) : 0;
  }

  int available() override {
    if (!_fp) {
      return 0;
    }
    long cur = ftell(_fp);
    long sz = (long)size();
    return (int)(sz - cur);
  }
  int read() override { return _fp ? fgetc(_fp) : -1; }
  size_t read(uint8_t* buf, size_t len) { return _fp ? fread(buf, 1, len, _fp) : 0; }
  int peek() override {
    if (!_fp) {
      return -1;
    }
    int c = fgetc(_fp);
    if (c != EOF) {
      ungetc(c, _fp);
    }
    return c;
  }

  size_t size() {
    if (!_fp) {
      return 0;
    }
    long cur = ftell(_fp);
    fseek(_fp, 0, SEEK_END);
    long sz = ftell(_fp);
    fseek(_fp, cur, SEEK_SET);
    return sz > 0 ? (size_t)sz : 0;
  }

  void close() {
    if (_fp) {
      fclose(_fp);
      _fp = nullptr;
    }
  }

private:
  FILE* _fp;
};

namespace fs {

class FS {
public:
  virtual ~FS() = default;
  virtual bool mkdir(const char* path) = 0;
  virtual bool exists(const char* path) = 0;
  virtual bool remove(const char* path) = 0;
  virtual File open(const char* path, const char* mode) = 0;
};

}  // namespace fs

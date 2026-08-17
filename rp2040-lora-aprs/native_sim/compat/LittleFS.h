#pragma once

#include "FS.h"

class LittleFSClass : public fs::FS {
public:
  bool begin();
  File open(const char* path, const char* mode) override;
  bool remove(const char* path) override;
  bool mkdir(const char* path) override;
  bool exists(const char* path) override;
  bool format() { return true; }
};

extern LittleFSClass LittleFS;

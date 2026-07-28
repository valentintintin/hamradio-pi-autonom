#pragma once

#include "FS.h"

// ============================================================================
// LittleFS.h — shim natif. Mêmes opérations que le vrai LittleFS
// (open/read/write/size/close/remove/exists/mkdir) mais adossées à de vrais
// fichiers POSIX sous le répertoire désigné par la variable d'env
// SIM_DATA_DIR (défaut "native-data/fs"), pour persister settings/identité/
// image SSTV entre deux lancements du simulateur comme le ferait la vraie
// flash.
// ============================================================================

class LittleFSClass : public fs::FS {
public:
  bool begin();
  File open(const char* path, const char* mode) override;
  bool remove(const char* path) override;
  bool mkdir(const char* path) override;
  bool exists(const char* path) override;
  bool format() { return true; }  // no-op — MeshCore (MyMesh::formatFileSystem)
};

extern LittleFSClass LittleFS;

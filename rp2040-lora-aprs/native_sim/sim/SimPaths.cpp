#include "SimPaths.h"

#include <cstdlib>
#include <sys/stat.h>

std::string simDataDir() {
  const char* env = getenv("SIM_DATA_DIR");
  return env ? env : "native-data";
}

void simMkdirs(const std::string& dir) {
  std::string path;
  for (size_t i = 0; i < dir.size(); i++) {
    path += dir[i];
    if (dir[i] == '/' || i == dir.size() - 1) {
      if (!path.empty()) {
        ::mkdir(path.c_str(), 0755);
      }
    }
  }
}

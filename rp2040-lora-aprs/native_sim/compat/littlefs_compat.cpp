#include "LittleFS.h"
#include "SimPaths.h"

#include <sys/stat.h>

LittleFSClass LittleFS;

namespace {

std::string dataDir() {
  return simDataDir() + "/littlefs";
}

std::string resolvePath(const char* path) {
  std::string dir = dataDir();
  simMkdirs(dir);
  std::string p = path ? path : "";
  if (!p.empty() && p[0] == '/') {
    p = p.substr(1);
  }
  return dir + "/" + p;
}

}  // namespace

bool LittleFSClass::begin() {
  simMkdirs(dataDir());
  return true;
}

File LittleFSClass::open(const char* path, const char* mode) {
  std::string full = resolvePath(path);
  FILE* fp = fopen(full.c_str(), mode);
  return File(fp);
}

bool LittleFSClass::remove(const char* path) {
  return ::remove(resolvePath(path).c_str()) == 0;
}

bool LittleFSClass::mkdir(const char* path) {
  simMkdirs(resolvePath(path));
  return true;
}

bool LittleFSClass::exists(const char* path) {
  struct stat st;
  return ::stat(resolvePath(path).c_str(), &st) == 0;
}

#include "SimWorld.h"

#include <cstdarg>
#include <cstdio>
#include <string>

// Implémente nativeLogTee() déclarée par core/Log.h sous NATIVE_BUILD —
// duplique chaque ligne de log dans SimWorld::log_ring pour le dashboard web.
void nativeLogTee(const char* prefix, const char* uptime, const char* tag, const char* fmt, ...) {
  char msg[256];
  va_list args;
  va_start(args, fmt);
  vsnprintf(msg, sizeof(msg), fmt, args);
  va_end(args);

  char line[300];
  snprintf(line, sizeof(line), "%s [%s] [%-8s] %s", prefix, uptime, tag, msg);

  auto& w = SimWorld::instance();
  std::lock_guard<std::mutex> lock(w.mutex);
  w.pushLogLine(line);
}

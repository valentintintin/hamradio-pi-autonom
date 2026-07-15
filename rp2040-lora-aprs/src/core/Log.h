#pragma once

#include <Arduino.h>

// ============================================================================
// Logger minimaliste avec niveaux
//
// Niveau configurable à runtime via g_log_level (lié aux Settings).
// Format : [TAG] message
//
// Usage :
//   LOG_E("RADIO", "Init fail: %d", state);
//   LOG_I("APRS", "Beacon envoyé");
//   LOG_D("WEATHER", "Cycle FSK terminé");
// ============================================================================

enum LogLevel : uint8_t {
  LOG_NONE  = 0,
  LOG_ERROR = 1,
  LOG_WARN  = 2,
  LOG_INFO  = 3,
  LOG_DEBUG = 4,
  LOG_TRACE = 5
};

extern LogLevel g_log_level;

// Prefixes par niveau
inline const char* logLevelPrefix(LogLevel lvl) {
  switch (lvl) {
    case LOG_ERROR: return "E";
    case LOG_WARN:  return "W";
    case LOG_INFO:  return "I";
    case LOG_DEBUG: return "D";
    case LOG_TRACE: return "T";
    default:        return "?";
  }
}

// Macro principale — compile le check de niveau avant le printf
#define LOG_MSG(lvl, tag, fmt, ...) \
  do { \
    if ((lvl) <= g_log_level) \
      Serial.printf("%s [%-8s] " fmt "\n", logLevelPrefix(lvl), tag, ##__VA_ARGS__); \
  } while(0)

#define LOG_E(tag, fmt, ...) LOG_MSG(LOG_ERROR, tag, fmt, ##__VA_ARGS__)
#define LOG_W(tag, fmt, ...) LOG_MSG(LOG_WARN,  tag, fmt, ##__VA_ARGS__)
#define LOG_I(tag, fmt, ...) LOG_MSG(LOG_INFO,  tag, fmt, ##__VA_ARGS__)
#define LOG_D(tag, fmt, ...) LOG_MSG(LOG_DEBUG, tag, fmt, ##__VA_ARGS__)
#define LOG_T(tag, fmt, ...) LOG_MSG(LOG_TRACE, tag, fmt, ##__VA_ARGS__)

// Helper pour parser un niveau depuis un string (pour CLI "set system.log_level debug")
inline LogLevel parseLogLevel(const char* s) {
  if (strcmp(s, "error") == 0 || strcmp(s, "1") == 0) {
    return LOG_ERROR;
  }
  if (strcmp(s, "warn") == 0 || strcmp(s, "2") == 0) {
    return LOG_WARN;
  }
  if (strcmp(s, "info") == 0 || strcmp(s, "3") == 0) {
    return LOG_INFO;
  }
  if (strcmp(s, "debug") == 0 || strcmp(s, "4") == 0) {
    return LOG_DEBUG;
  }
  if (strcmp(s, "trace") == 0 || strcmp(s, "5") == 0) {
    return LOG_TRACE;
  }
  if (strcmp(s, "none") == 0 || strcmp(s, "0") == 0) {
    return LOG_NONE;
  }
  return LOG_INFO; // défaut
}

inline const char* logLevelName(LogLevel lvl) {
  switch (lvl) {
    case LOG_NONE:  return "none";
    case LOG_ERROR: return "error";
    case LOG_WARN:  return "warn";
    case LOG_INFO:  return "info";
    case LOG_DEBUG: return "debug";
    case LOG_TRACE: return "trace";
    default:        return "?";
  }
}

#pragma once

#include <Arduino.h>

// task_watchdog.cpp ne nourrit le chien que si toutes les tâches sont à
// jour : le blocage d'une seule déclenche un reboot correcteur, même si la
// tâche watchdog elle-même tourne encore normalement.

enum HeartbeatTask {
  HB_MESH = 0,
  HB_APRS,
  HB_BEACON,
  HB_SENSORS,
  HB_ENERGY,
  HB_WEATHER,
  HB_CLI,
  HB_COUNT
};

extern volatile unsigned long g_heartbeat_ms[HB_COUNT];

inline void heartbeat(HeartbeatTask task) {
  g_heartbeat_ms[task] = millis();
}

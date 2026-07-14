#pragma once

#include <Arduino.h>

// ============================================================================
// Heartbeat inter-tâches — chaque tâche surveillée signale qu'elle progresse
// encore (un appel par itération de sa boucle). task_watchdog.cpp ne nourrit
// le chien matériel que si toutes sont à jour : un blocage d'une seule tâche
// (pas forcément tout le système) déclenche donc un reboot correcteur, au
// lieu d'être masqué par le fait que la tâche watchdog elle-même tourne
// encore normalement.
// ============================================================================

enum HeartbeatTask {
  HB_MESH = 0,
  HB_APRS,
  HB_BEACON,
  HB_ENERGY,
  HB_WEATHER,
  HB_CLI,
  HB_COUNT
};

extern volatile unsigned long g_heartbeat_ms[HB_COUNT];

inline void heartbeat(HeartbeatTask task) {
  g_heartbeat_ms[task] = millis();
}

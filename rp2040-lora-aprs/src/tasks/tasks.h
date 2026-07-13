#pragma once

// ============================================================================
// Tasks FreeRTOS — déclarations
// ============================================================================

#include <Arduino.h>

// Priorités (plus haut = plus prioritaire)
#define TASK_PRIO_MESH_LOOP     4   // Dispatcher MeshCore — haute prio (perte de paquets sinon)
#define TASK_PRIO_APRS_LOOP     4   // Dispatcher APRS — haute prio aussi
#define TASK_PRIO_BEACON        1   // Beacons APRS périodiques
#define TASK_PRIO_ENERGY        1   // Monitoring batterie/solaire
#define TASK_PRIO_WEATHER       3   // WH65B FSK — prio > beacon, < radio loops
#define TASK_PRIO_CLI           1   // Commandes série

// Stack sizes (en words, 1 word = 4 bytes sur ARM)
#define TASK_STACK_MESH         4096
#define TASK_STACK_APRS         4096
#define TASK_STACK_BEACON       2048
#define TASK_STACK_ENERGY       1024
#define TASK_STACK_WEATHER      2048
#define TASK_STACK_CLI          2048

// Task functions
void taskMeshLoop(void* params);
void taskAprsLoop(void* params);
void taskAprsBeacon(void* params);
void taskEnergy(void* params);
void taskWeather(void* params);
void taskCli(void* params);

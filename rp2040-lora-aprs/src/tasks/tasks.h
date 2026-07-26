#pragma once

// ============================================================================
// Tasks FreeRTOS — déclarations
// ============================================================================

#include <Arduino.h>

// Priorités (plus haut = plus prioritaire)
#define TASK_PRIO_MESH_LOOP     4   // Dispatcher MeshCore — haute prio (perte de paquets sinon)
#define TASK_PRIO_APRS_LOOP     4   // Dispatcher APRS — haute prio aussi
#define TASK_PRIO_BEACON        1   // Beacons APRS périodiques
#define TASK_PRIO_SENSORS       1   // Lecture capteurs (INA3221/MPPT/BME280/Victron) + historique
#define TASK_PRIO_ENERGY        1   // Supervision énergie (watchdog MPPT, coupure relais sur sous-tension)
#define TASK_PRIO_WEATHER       3   // WH65B FSK — prio > beacon, < radio loops
#define TASK_PRIO_CLI           1   // Commandes série
#define TASK_PRIO_WATCHDOG      1   // Nourrit le watchdog matériel RP2040

// Stack sizes (en words, 1 word = 4 bytes sur ARM)
#define TASK_STACK_MESH         4096
#define TASK_STACK_APRS         4096
#define TASK_STACK_BEACON       2048
#define TASK_STACK_SENSORS      1024
#define TASK_STACK_ENERGY       512
#define TASK_STACK_WEATHER      2048
#define TASK_STACK_CLI          2048
#define TASK_STACK_WATCHDOG     512

// Task functions
void taskMeshLoop(void* params);
void taskAprsLoop(void* params);
void taskAprsBeacon(void* params);
void taskSensors(void* params);
void taskEnergy(void* params);
void taskWeather(void* params);
void taskCli(void* params);
void taskWatchdog(void* params);

#pragma once

// ============================================================================
// Boot — séquence d'initialisation détaillée de setup() (cf. main.cpp), une
// fonction par étape pour rester lisible. Les objets globaux (radios, bus
// I2C, HAL, tâches FreeRTOS...) restent déclarés dans main.cpp — racine de
// composition de l'appli — et sont référencés ici par extern, comme le fait
// déjà chaque tasks/task_*.cpp. Appelées dans cet ordre par setup().
// ============================================================================

void bootInitCore();          // Serial, board, filesystem
void bootInitRadios();        // MeshCore + APRS (SX1262 x2)
void bootSeedRng();           // RNG depuis bruit radio
void bootInitIdentity();      // Identité MeshCore (charge ou génère)
void bootInitSensors();       // Bus I2C + tous les capteurs/HAL + choix du chargeur actif
void bootLoadConfig();        // Settings (LittleFS > EEPROM > défauts)
void bootInitRelays();        // Relais bistables (dépend de bootLoadConfig)
void bootInitMeshAndAprs();   // Démarre MeshCore + APRS, advert initial
void bootCreateTasks();       // Toutes les tâches FreeRTOS

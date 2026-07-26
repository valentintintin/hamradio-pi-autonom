#pragma once

// ============================================================================
// Boot — séquence d'initialisation détaillée de setup() (cf. main.cpp), une
// fonction par étape pour rester lisible. Les objets globaux (radios, bus
// I2C, HAL, tâches FreeRTOS...) restent déclarés dans main.cpp — racine de
// composition de l'appli — et sont référencés ici par extern, comme le fait
// déjà chaque tasks/task_*.cpp. Appelées dans cet ordre par setup().
//
// Modes de fonctionnement (cf. config/Settings.h: OperatingMode) — la carte
// peut être déployée en standalone (juste APRS, ou juste MeshCore) ou en
// fonctionnement normal (APRS + MeshCore + relais + télémétrie) :
//   - bootInitEeprom : toujours exécutée (I2C bus + EEPROM), quel que soit le
//     mode — nécessaire pour le fallback EEPROM du chargement des settings
//     (cf. bootLoadConfig), donc appelée AVANT que le mode ne soit connu.
//   - bootInitRadios/bootSeedRng/bootInitIdentity/bootInitSensors/
//     bootInitRelays/bootInitMeshAndAprs/bootCreateTasks : chacune ne fait
//     que ce que le mode réellement configuré (settings.system.mode)
//     nécessite — pas de tâche FreeRTOS ni de radio inutiles en standalone.
// ============================================================================

void bootInitCore();          // Serial, board, filesystem
void bootInitEeprom();        // Bus I2C + EEPROM seuls (toujours, cf. ci-dessus)
void bootLoadConfig();        // Settings (LittleFS > EEPROM > défauts) — settings.system.mode connu après
void bootInitRadios();        // MeshCore et/ou APRS (SX1262 x2) selon le mode
void bootSeedRng();           // RNG depuis bruit radio — si MeshCore actif
void bootInitIdentity();      // Identité MeshCore (charge ou génère) — si MeshCore actif
void bootInitSensors();       // Capteurs/chargeurs/historique EEPROM — si mode complet
void bootInitRelays();        // Relais bistables — si mode complet
void bootInitMeshAndAprs();   // Démarre MeshCore et/ou APRS selon le mode, advert initial
void bootCreateTasks();       // Tâches FreeRTOS pertinentes pour le mode (CLI/watchdog toujours)

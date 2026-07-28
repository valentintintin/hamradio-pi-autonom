#pragma once

#include <string>

// ============================================================================
// SimPaths — résolution partagée du répertoire de données du simulateur
// (variable d'env SIM_DATA_DIR, défaut "native-data"), utilisé à la fois par
// native_sim/compat/LittleFS.h (sous-dossier "littlefs/") et
// hal/eeprom/M24M01Hal.cpp en NATIVE_BUILD (sous-dossier "eeprom/").
// ============================================================================

// Racine des données simulées (SIM_DATA_DIR ou "native-data" par défaut).
std::string simDataDir();

// Crée récursivement tous les répertoires parents de `dir` (POSIX mkdir ne
// crée qu'un niveau à la fois).
void simMkdirs(const std::string& dir);

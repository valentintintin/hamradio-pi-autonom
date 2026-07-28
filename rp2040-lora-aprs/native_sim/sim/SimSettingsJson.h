#pragma once

#include <string>
#include "config/Settings.h"

// ============================================================================
// SimSettingsJson — (dé)sérialisation Settings <-> JSON, générique via
// SettingsRegistry (la même table clé/type/pointeur que le CLI get/set/list,
// cf. config/SettingsRegistry.h) plutôt qu'un dump binaire brut — utilisé par
// config/SettingsManager.h et hal/eeprom/M24M01Hal.cpp en NATIVE_BUILD pour
// que les fichiers de settings simulés (SIM_DATA_DIR/littlefs/settings.json,
// SIM_DATA_DIR/eeprom/settings.json) soient lisibles/éditables directement,
// au lieu d'un blob opaque. Générique : reste à jour tout seul si Settings.h
// évolue, aucun champ n'est connu ici individuellement.
// ============================================================================

std::string settingsToJson(const Settings& s);

// Retourne false si le JSON est invalide. Sur succès, `s` est d'abord
// réinitialisée à getDefaultSettings() puis chaque clé présente dans le JSON
// est appliquée par-dessus — une clé absente (champ ajouté depuis) retombe
// donc sur sa valeur par défaut plutôt que de rester à zéro.
bool settingsFromJson(const std::string& json, Settings& s);

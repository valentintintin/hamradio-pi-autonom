#pragma once

#include <string>
#include "config/Settings.h"

std::string settingsToJson(const Settings& s);

// `s` est réinitialisée à getDefaultSettings() avant application du JSON : une
// clé absente (champ ajouté depuis) retombe sur sa valeur par défaut, pas sur zéro.
bool settingsFromJson(const std::string& json, Settings& s);

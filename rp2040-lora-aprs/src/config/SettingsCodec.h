#pragma once

#include "Settings.h"

// Deux implémentations sélectionnées par platformio.ini selon l'env (jamais de #ifdef ici) :
// src/config/SettingsCodec.cpp (struct binaire, cible réelle) vs native_sim/sim/SettingsCodecSim.cpp (JSON).
bool settingsLoadFromLittleFS(Settings& s);
bool settingsSaveToLittleFS(const Settings& s);

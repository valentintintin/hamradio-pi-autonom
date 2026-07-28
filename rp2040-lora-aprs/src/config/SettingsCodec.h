#pragma once

#include "Settings.h"

// ============================================================================
// SettingsCodec — sérialisation des Settings vers/depuis LittleFS.
//
// Deux implémentations, une par environnement (même principe que variant/
// vs variant_native/ pour les radios, ou hal/eeprom/M24M01Hal.cpp vs
// native_sim/sim/M24M01HalSim.cpp) : platformio.ini exclut celle qui ne
// correspond pas à l'env courant, donc jamais de #ifdef NATIVE_BUILD ici ni
// dans SettingsManager.h.
//   - src/config/SettingsCodec.cpp        : struct binaire brute (cible réelle)
//   - native_sim/sim/SettingsCodecSim.cpp  : JSON lisible/éditable (env `native`,
//                                            cf. native_sim/sim/SimSettingsJson.h)
// ============================================================================

bool settingsLoadFromLittleFS(Settings& s);
bool settingsSaveToLittleFS(const Settings& s);

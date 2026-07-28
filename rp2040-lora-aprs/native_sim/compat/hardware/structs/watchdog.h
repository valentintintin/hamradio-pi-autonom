#pragma once

#include <cstdint>

// ============================================================================
// hardware/structs/watchdog.h — shim natif de la struct registre pico-sdk
// utilisée par tasks/task_watchdog.cpp comme compteur persistant de reboots
// watchdog consécutifs (watchdog_hw->scratch[0]). Pas de persistance réelle
// entre deux lancements du binaire natif (contrairement au vrai registre
// scratch, qui survit à un reset matériel) : chaque lancement repart à zéro,
// ce qui est le comportement voulu en dev (pas de "boucle de reboot" à
// simuler).
// ============================================================================

struct watchdog_hw_t {
  uint32_t scratch[8] = {0};
};

extern watchdog_hw_t* watchdog_hw;

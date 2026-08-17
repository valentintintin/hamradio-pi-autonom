#pragma once

#include <cstdint>

// Contrairement au vrai registre scratch, ne survit pas au reset: chaque
// lancement repart à zéro (voulu, pas de boucle de reboot à simuler en dev).
struct watchdog_hw_t {
  uint32_t scratch[8] = {0};
};

extern watchdog_hw_t* watchdog_hw;

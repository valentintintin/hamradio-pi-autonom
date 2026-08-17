#pragma once

// watchdog_hw->scratch[0..7] survivent à un reset "à chaud" mais pas à un POR/BOR : ne jamais les lire comme
// valides sans vérifier rp2040.getResetReason() == RP2040::WDT_RESET d'abord. scratch[4..7] réservés par
// pico-sdk (watchdog_reboot()) — ne pas y toucher.
#define SCRATCH_WDT_REBOOT_COUNT   0  // tasks/task_watchdog.cpp : compteur de reboots watchdog consécutifs
#define SCRATCH_RELAY_STATE_MASK   1  // hal/relay/RelayHal.cpp   : bit i = état ON/OFF du relais i (0..RELAY_COUNT-1)

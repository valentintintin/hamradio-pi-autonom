#pragma once

// ============================================================================
// ScratchRegisters — table centrale des index watchdog_hw->scratch[0..7]
// utilisés par ce firmware.
//
// Ces 8 registres 32 bits (périphérique WATCHDOG du RP2040, domaine
// "always-on") survivent à un reset RP2040 "à chaud" — watchdog matériel
// (timeout réel) OU rp2040.reboot()/"reboot"/"dfu" (qui passe par
// watchdog_reboot(), cf. framework-arduinopico/cores/rp2040/RP2040Support.h)
// — contrairement à la RAM normale, réinitialisée par le runtime C à chaque
// démarrage. Ils sont en revanche bien effacés par une vraie coupure
// d'alimentation (POR/BOR) : ne jamais les lire comme valides sans d'abord
// vérifier rp2040.getResetReason() == RP2040::WDT_RESET (cf.
// tasks/task_watchdog.cpp, seul moyen fiable de distinguer un reset "à chaud"
// d'un boot à froid où ce registre lirait 0/garbage).
//
// scratch[4..7] sont réservés par pico-sdk (hardware_watchdog::
// watchdog_reboot(), utilisé par rp2040.reboot() lui-même) — ne jamais les
// réutiliser ici, même si ce firmware n'appelle pas cette fonction
// directement.
//
// Ajouter un nouvel usage : prendre le prochain index libre ci-dessous et
// documenter son rôle, comme les deux existants.
// ============================================================================

#define SCRATCH_WDT_REBOOT_COUNT   0  // tasks/task_watchdog.cpp : compteur de reboots watchdog consécutifs
#define SCRATCH_RELAY_STATE_MASK   1  // hal/relay/RelayHal.cpp   : bit i = état ON/OFF du relais i (0..RELAY_COUNT-1)

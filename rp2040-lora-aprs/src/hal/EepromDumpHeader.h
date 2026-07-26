#pragma once

#include <stdint.h>

// ============================================================================
// EepromDumpHeader — préfixe binaire commun aux dumps EEPROM efficaces
// (cf. TelemetryHistory::dumpBinary, EventLogHistory::dumpBinary, CLI
// "history dump" / "eventlog dump"). Écrit une seule fois en tête du flux,
// suivi de `count` records bruts de `record_size` bytes chacun (packed,
// ordre : le plus ancien en premier) — permet à l'outil côté PC de savoir
// combien lire et leur taille sans rien décoder au préalable.
// ============================================================================
struct __attribute__((packed)) EepromDumpHeader {
  uint32_t magic;
  uint16_t record_size;
  uint16_t count;
};

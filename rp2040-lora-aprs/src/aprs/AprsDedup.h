#pragma once

#include <Aprs.h>
#include <stdint.h>

// APRS101 §4.2a : fenêtre de dédup digipeat ~30s
#define APRS_DEDUP_SLOTS       16
#define APRS_DEDUP_WINDOW_MS   30000

class AprsDedup {
public:
  AprsDedup();

  uint32_t hash(const aprs::PacketLite& p) const;

  bool isDuplicate(uint32_t hash, uint32_t now) const;
  void remember(uint32_t hash, uint32_t now);

private:
  struct DedupEntry {
    uint32_t hash;
    uint32_t timeMs;
    // Sans ce flag, un paquet de hash 0 serait pris pour un doublon pendant
    // les 30s suivant le boot (slots initialisés à {0,0}).
    bool valid;
  };
  DedupEntry _slots[APRS_DEDUP_SLOTS];
};

#pragma once

#include <Aprs.h>
#include <stdint.h>

// Suppression des doublons digipeat — APRS Digipeater Algorithm §4.2a (~30s)
#define APRS_DEDUP_SLOTS       16
#define APRS_DEDUP_WINDOW_MS   30000

// ============================================================================
// AprsDedup — suppression des doublons digipeat (APRS101 §4.2a)
//
// `valid` distingue un slot jamais utilisé d'une entrée réelle : sans ça, un
// paquet dont le hash vaut exactement 0 serait pris pour un doublon pendant
// les 30 premières secondes après boot (tous les slots démarrent à
// {hash:0, timeMs:0}).
// ============================================================================
class AprsDedup {
public:
  AprsDedup();

  // Hash source+destination(sans SSID)+contenu d'une trame décodée
  uint32_t hash(const aprs::PacketLite& p) const;

  bool isDuplicate(uint32_t hash, uint32_t now) const;
  void remember(uint32_t hash, uint32_t now);

private:
  struct DedupEntry {
    uint32_t hash;
    uint32_t timeMs;
    bool valid;
  };
  DedupEntry _slots[APRS_DEDUP_SLOTS];
};

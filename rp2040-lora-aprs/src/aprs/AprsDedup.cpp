#include "AprsDedup.h"
#include <string.h>

AprsDedup::AprsDedup() {
  memset(_slots, 0, sizeof(_slots));
}

uint32_t AprsDedup::hash(const aprs::PacketLite& p) const {
  char destNoSsid[aprs::kCallsignLength + 1];
  size_t i = 0;
  for (; p.destination[i] && p.destination[i] != '-' && i < sizeof(destNoSsid) - 1; i++) {
    destNoSsid[i] = p.destination[i];
  }
  destNoSsid[i] = '\0';

  uint32_t h = 5381;
  for (const char* s = p.source; *s; s++) {
    h = (h * 33) ^ (uint8_t)*s;
  }
  for (const char* s = destNoSsid; *s; s++) {
    h = (h * 33) ^ (uint8_t)*s;
  }
  for (const char* s = p.content; *s; s++) {
    h = (h * 33) ^ (uint8_t)*s;
  }
  return h;
}

bool AprsDedup::isDuplicate(uint32_t hash, uint32_t now) const {
  for (uint8_t i = 0; i < APRS_DEDUP_SLOTS; i++) {
    if (_slots[i].valid && _slots[i].hash == hash && (now - _slots[i].timeMs) < APRS_DEDUP_WINDOW_MS) {
      return true;
    }
  }
  return false;
}

void AprsDedup::remember(uint32_t hash, uint32_t now) {
  uint8_t oldest = 0;
  bool oldest_valid = _slots[0].valid;
  for (uint8_t i = 1; i < APRS_DEDUP_SLOTS; i++) {
    if (!_slots[i].valid) {
      oldest = i;
      oldest_valid = false;
      break;
    }
    if (oldest_valid && _slots[i].timeMs < _slots[oldest].timeMs) {
      oldest = i;
    }
  }
  _slots[oldest].hash = hash;
  _slots[oldest].timeMs = now;
  _slots[oldest].valid = true;
}

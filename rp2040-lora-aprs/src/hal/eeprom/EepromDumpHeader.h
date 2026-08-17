#pragma once

#include <stdint.h>

// Précède un dump binaire : `count` records de `record_size` bytes suivent,
// du plus ancien au plus récent.
struct __attribute__((packed)) EepromDumpHeader {
  uint32_t magic;
  uint16_t record_size;
  uint16_t count;
};

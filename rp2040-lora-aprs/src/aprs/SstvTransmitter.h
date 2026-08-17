#pragma once

#include <Arduino.h>
#include <LittleFS.h>
#include <stdint.h>
#include "config/SettingsRegistry.h"  // EnumNameEntry
#include "config/Settings.h"

// La séquence de transmission (pause dispatcher, CW, SSTV, CW, ~1-2 min)
// tourne exclusivement dans tasks/task_sstv.cpp, jamais dans le mutex
// partagé CommandHandler/AprsEngine, pour ne pas bloquer CLI/APRS.

#define SSTV_IMAGE_PATH "/sstv_image.bin"

// Pasokon (640x496, ~930 Ko) exclu : ne tient pas dans la partition LittleFS
// actuelle (board_build.filesystem_size = 0.5m).
#define SSTV_MODE_LIST(X) \
  X(0, "robot36",   Robot36,   320, 240) \
  X(1, "robot72",   Robot72,   320, 240) \
  X(2, "martin1",   Martin1,   320, 256) \
  X(3, "martin2",   Martin2,   320, 256) \
  X(4, "scottie1",  Scottie1,  320, 256) \
  X(5, "scottie2",  Scottie2,  320, 256) \
  X(6, "scottiedx", ScottieDX, 320, 256) \
  X(7, "wrasse",    Wrasse,    320, 256)

#define SSTV_MODE_COUNT 8

extern const EnumNameEntry SSTV_MODE_NAMES[SSTV_MODE_COUNT];

class SstvTransmitter {
public:
  void init(Settings& settings) { _settings = &settings; }

  uint32_t expectedImageBytes() const;

  bool beginImageUpload(uint32_t announced_bytes, Print* out);
  bool writeImageChunk(const uint8_t* data, size_t len);
  bool isUploadComplete() const { return _upload_complete; }
  uint32_t uploadWrittenBytes() const { return _upload_written_bytes; }
  void cancelUpload();

  // Arme un flag et rend la main immédiatement, ne bloque jamais.
  bool requestTransmit(Print* out);

  // Consomme (et efface) le flag armé par requestTransmit() ; appelée uniquement par taskSstv.
  bool consumeTransmitRequest();

  // Séquence bloquante CW+SSTV+CW (~1-2 min) ; appelée uniquement par taskSstv.
  void transmit();

private:
  Settings* _settings = nullptr;

  File _upload_file;
  uint32_t _upload_expected_bytes = 0;
  uint32_t _upload_written_bytes = 0;
  bool _upload_open = false;
  bool _upload_complete = false;

  volatile bool _transmit_requested = false;
};

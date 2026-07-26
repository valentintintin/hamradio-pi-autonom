#pragma once

#include <Arduino.h>
#include <LittleFS.h>
#include <stdint.h>
#include "config/SettingsRegistry.h"  // EnumNameEntry
#include "config/Settings.h"

// ============================================================================
// SstvTransmitter — upload d'image (streaming LittleFS) + émission CW+SSTV
//
// Un PC uploade une image en binaire par le CLI série ("image <n>", cf.
// tasks/task_cli.cpp), écrite en streaming sur LittleFS (aucun mode SSTV ne
// tient en RAM libre du RP2040). "image send" (local OU distant APRS/mesh,
// cf. cli/CommandHandler.cpp) ne fait qu'armer une demande via
// requestTransmit() et rend la main immédiatement : la séquence bloquante
// (pause dispatcher APRS, CW indicatif, SSTV, CW indicatif, retour LoRa,
// 1-2 minutes selon le mode) tourne exclusivement dans tasks/task_sstv.cpp,
// hors du mutex partagé CommandHandler/AprsEngine (cf. plan).
//
// Table des modes SSTV exposés : seule celles qui tiennent dans la partition
// LittleFS actuelle (board_build.filesystem_size = 0.5m, cf. platformio.ini)
// en RGB888 brut — Pasokon (640x496, ~930 Ko) est exclu.
// ============================================================================

#define SSTV_IMAGE_PATH "/sstv_image.bin"

// index, nom CLI, mode RadioLib (variable extern de RadioLib/protocols/SSTV/SSTV.h),
// largeur, hauteur — source unique, réutilisée pour construire la table
// noms<->index consommée par SettingsRegistry ("sstv.mode") et la table
// index->{mode RadioLib, dimensions} utilisée par SstvTransmitter.
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

// Table nom<->index consommée par SettingsRegistry (ST_ENUM8, "sstv.mode")
extern const EnumNameEntry SSTV_MODE_NAMES[SSTV_MODE_COUNT];

class SstvTransmitter {
public:
  void init(Settings& settings) { _settings = &settings; }

  // Taille attendue (octets RGB888, sans en-tête) pour le mode actuellement
  // configuré (settings.cw_sstv.sstv_mode).
  uint32_t expectedImageBytes() const;

  // --- Upload (appelé depuis tasks/task_cli.cpp, thread CLI, série uniquement) ---
  bool beginImageUpload(uint32_t announced_bytes, Print* out);
  bool writeImageChunk(const uint8_t* data, size_t len);
  bool isUploadComplete() const { return _upload_complete; }
  uint32_t uploadWrittenBytes() const { return _upload_written_bytes; }
  void cancelUpload();

  // --- Requête/exécution transmission ---------------------------------------
  // Appelée depuis CommandHandler ("image send", local ET distant APRS/mesh) :
  // arme un flag et rend la main immédiatement, ne bloque JAMAIS.
  bool requestTransmit(Print* out);

  // Appelée uniquement par taskSstv : consomme (et efface) le flag armé par
  // requestTransmit().
  bool consumeTransmitRequest();

  // Séquence bloquante CW+SSTV+CW (1-2 minutes selon le mode) — appelée
  // uniquement par taskSstv, jamais par CommandHandler (cf. commentaire de
  // fichier).
  void transmit();

private:
  Settings* _settings = nullptr;

  File _upload_file;
  uint32_t _upload_expected_bytes = 0;
  uint32_t _upload_written_bytes = 0;
  bool _upload_open = false;
  bool _upload_complete = false;

  volatile bool _transmit_requested = false;

  void sendCallsignMorse(class MorseClient& morse);
};

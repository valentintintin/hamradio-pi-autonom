#include "SstvTransmitter.h"
#include "LoRa433RadioMode.h"
#include "AprsDispatcher.h"
#include "core/Log.h"
#include "target.h"
#include <RadioLib.h>

extern AprsDispatcher aprs_dispatcher;

#define TAG "SSTV"

// ============================================================================
// Table nom<->index (SettingsRegistry, ST_ENUM8, "sstv.mode")
// ============================================================================
#define SSTV_MODE_NAME_ENTRY(idx, name, mode, w, h) { name, idx },
const EnumNameEntry SSTV_MODE_NAMES[SSTV_MODE_COUNT] = {
  SSTV_MODE_LIST(SSTV_MODE_NAME_ENTRY)
};
#undef SSTV_MODE_NAME_ENTRY

// ============================================================================
// Table index->{mode RadioLib, dimensions} (émission)
// ============================================================================
struct SstvModeInfo {
  const char* name;
  const SSTVMode_t* mode;
  uint16_t width;
  uint16_t height;
};

#define SSTV_MODE_INFO_ENTRY(idx, name, mode, w, h) { name, &mode, w, h },
static const SstvModeInfo SSTV_MODES[SSTV_MODE_COUNT] = {
  SSTV_MODE_LIST(SSTV_MODE_INFO_ENTRY)
};
#undef SSTV_MODE_INFO_ENTRY

static const SstvModeInfo& currentModeInfo(const Settings* settings) {
  uint8_t idx = settings->cw_sstv.sstv_mode;
  if (idx >= SSTV_MODE_COUNT) {
    idx = 0;
  }
  return SSTV_MODES[idx];
}

// ============================================================================
// Taille attendue pour le mode courant
// ============================================================================
uint32_t SstvTransmitter::expectedImageBytes() const {
  const SstvModeInfo& info = currentModeInfo(_settings);
  return (uint32_t)info.width * (uint32_t)info.height * 3;
}

// ============================================================================
// Upload (thread CLI, série uniquement)
// ============================================================================
bool SstvTransmitter::beginImageUpload(uint32_t announced_bytes, Print* out) {
  uint32_t expected = expectedImageBytes();
  if (announced_bytes != expected) {
    if (out) {
      out->printf("Taille attendue pour le mode courant (%s): %lu octets (reçu: %lu)\n",
        currentModeInfo(_settings).name, (unsigned long)expected, (unsigned long)announced_bytes);
    }
    return false;
  }

  if (_upload_open) {
    _upload_file.close();
    _upload_open = false;
  }
  LittleFS.remove(SSTV_IMAGE_PATH);

  _upload_file = LittleFS.open(SSTV_IMAGE_PATH, "w");
  if (!_upload_file) {
    if (out) {
      out->println(F("Erreur ouverture fichier image sur LittleFS"));
    }
    return false;
  }

  _upload_expected_bytes = expected;
  _upload_written_bytes = 0;
  _upload_open = true;
  _upload_complete = false;
  return true;
}

bool SstvTransmitter::writeImageChunk(const uint8_t* data, size_t len) {
  if (!_upload_open || _upload_complete) {
    return false;
  }

  uint32_t remaining = _upload_expected_bytes - _upload_written_bytes;
  size_t toWrite = (len > remaining) ? remaining : len;

  if (toWrite > 0) {
    size_t written = _upload_file.write(data, toWrite);
    _upload_written_bytes += written;
    if (written != toWrite) {
      LOG_E(TAG, "Écriture LittleFS incomplète (%u/%u)", (unsigned)written, (unsigned)toWrite);
      return false;
    }
  }

  if (_upload_written_bytes >= _upload_expected_bytes) {
    _upload_file.close();
    _upload_open = false;
    _upload_complete = true;
  }
  return true;
}

void SstvTransmitter::cancelUpload() {
  if (_upload_open) {
    _upload_file.close();
    _upload_open = false;
  }
  _upload_complete = false;
  _upload_written_bytes = 0;
  LittleFS.remove(SSTV_IMAGE_PATH);
}

// ============================================================================
// Requête/exécution transmission
// ============================================================================
bool SstvTransmitter::requestTransmit(Print* out) {
  if (!_upload_complete) {
    if (out) {
      out->println(F("Aucune image complète en attente ('image <n>' d'abord)"));
    }
    return false;
  }
  _transmit_requested = true;
  if (out) {
    out->println(F("Transmission CW+SSTV programmée"));
  }
  return true;
}

bool SstvTransmitter::consumeTransmitRequest() {
  if (!_transmit_requested) {
    return false;
  }
  _transmit_requested = false;
  return true;
}

// switchToFsk() attend un callback RX (cf. LoRa433RadioMode.h) ; on ne reçoit
// jamais pendant une émission CW/SSTV (aucun startReceive() appelé), et
// directMode() (déclenché par MorseClient/SSTVClient::begin()) ne réarme de
// toute façon que l'IRQ TX_DONE — ce callback ne sera donc jamais invoqué.
static void noRxCallback() {}

void SstvTransmitter::sendCallsignMorse(MorseClient& morse) {
  for (uint8_t i = 0; i < _settings->cw_sstv.cw_repeats; i++) {
    morse.println(_settings->aprs.callsign);
  }
}

// ============================================================================
// Séquence bloquante CW + SSTV + CW (appelée uniquement par taskSstv)
// ============================================================================
void SstvTransmitter::transmit() {
  const SstvModeInfo& info = currentModeInfo(_settings);
  LOG_I(TAG, "Début transmission (mode=%s, %dx%d)", info.name, info.width, info.height);

  aprs_dispatcher.pause();

  // Réutilise switchToFsk() (déjà validée en réception WH65B réelle) plutôt
  // qu'une config FSK dédiée : seul le fait d'être en modem GFSK compte pour
  // transmitDirect()/directMode() (cf. LoRa433RadioMode.h), pas la fréquence
  // WH65B_FREQ qu'elle configure — MorseClient/SSTVClient retendent la
  // porteuse eux-mêmes sur la vraie fréquence CW/SSTV à chaque symbole. La
  // puissance, elle, n'est pas retouchée par symbole : à réappliquer ici.
  bool ok = LoRa433RadioMode::switchToFsk(noRxCallback);
  if (ok) {
    aprs_radio_hw.setOutputPower(_settings->cw_sstv.power_dbm);

    MorseClient morse(&aprs_radio_hw);
    morse.begin(_settings->cw_sstv.freq_mhz, _settings->cw_sstv.cw_wpm);
    sendCallsignMorse(morse);

    SSTVClient sstv(&aprs_radio_hw);
    int16_t state = sstv.begin(_settings->cw_sstv.freq_mhz, *info.mode);
    if (state == RADIOLIB_ERR_NONE) {
      sstv.sendHeader();

      File img = LittleFS.open(SSTV_IMAGE_PATH, "r");
      if (img) {
        static uint32_t line[640];  // max largeur supportée (modes actuels: 320px)
        static uint8_t rgb[640 * 3];
        for (uint16_t y = 0; y < info.height; y++) {
          size_t n = img.read(rgb, (size_t)info.width * 3);
          if (n != (size_t)info.width * 3) {
            LOG_E(TAG, "Lecture image incomplète ligne %d (%u/%u octets)", y, (unsigned)n, (unsigned)(info.width * 3));
            break;
          }
          for (uint16_t x = 0; x < info.width; x++) {
            line[x] = ((uint32_t)rgb[x * 3] << 16) | ((uint32_t)rgb[x * 3 + 1] << 8) | (uint32_t)rgb[x * 3 + 2];
          }
          sstv.sendLine(line);
        }
        img.close();
      } else {
        LOG_E(TAG, "Impossible de relire l'image uploadée");
      }
    } else {
      LOG_E(TAG, "SSTVClient::begin: %d", state);
    }

    sendCallsignMorse(morse);
    aprs_radio_hw.standby();
  }

  // switchToLora() repart des settings (radio.aprs.*), y compris la
  // puissance : pas de reapplication manuelle nécessaire ici.
  LoRa433RadioMode::switchToLora();
  aprs_dispatcher.resume();

  LittleFS.remove(SSTV_IMAGE_PATH);
  _upload_complete = false;
  _upload_written_bytes = 0;

  LOG_I(TAG, "Transmission terminée");
}

#include "SstvTransmitter.h"
#include "AprsDispatcher.h"
#include "core/Log.h"
#include "target.h"

extern AprsDispatcher aprs_dispatcher;

#define TAG "SSTV"

struct SstvModeInfo {
  const char* name;
  uint16_t width;
  uint16_t height;
};

#define SSTV_MODE_NAME_ENTRY(idx, name, mode, w, h) { name, idx },
const EnumNameEntry SSTV_MODE_NAMES[SSTV_MODE_COUNT] = {
  SSTV_MODE_LIST(SSTV_MODE_NAME_ENTRY)
};
#undef SSTV_MODE_NAME_ENTRY

#define SSTV_MODE_INFO_ENTRY(idx, name, mode, w, h) { name, w, h },
static const SstvModeInfo SSTV_MODES[SSTV_MODE_COUNT] = {
  SSTV_MODE_LIST(SSTV_MODE_INFO_ENTRY)
};
#undef SSTV_MODE_INFO_ENTRY

static uint8_t currentModeIndex(const Settings* settings) {
  uint8_t idx = settings->cw_sstv.sstv_mode;
  return idx < SSTV_MODE_COUNT ? idx : 0;
}

static const SstvModeInfo& currentModeInfo(const Settings* settings) {
  return SSTV_MODES[currentModeIndex(settings)];
}

uint32_t SstvTransmitter::expectedImageBytes() const {
  const SstvModeInfo& info = currentModeInfo(_settings);
  return (uint32_t)info.width * (uint32_t)info.height * 3;
}

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
      out->println("Erreur ouverture fichier image sur LittleFS");
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

bool SstvTransmitter::requestTransmit(Print* out) {
  if (!_upload_complete) {
    if (out) {
      out->println("Aucune image en attente");
    }
    return false;
  }
  _transmit_requested = true;
  if (out) {
    out->println("Transmission CW+SSTV programmée");
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

void SstvTransmitter::transmit() {
  const SstvModeInfo& info = currentModeInfo(_settings);
  LOG_I(TAG, "Début transmission (mode=%s, %dx%d)", info.name, info.width, info.height);

  aprs_dispatcher.pause();

  File img = LittleFS.open(SSTV_IMAGE_PATH, "r");
  if (img) {
    aprs_carrier.transmitCwSstv(_settings->aprs.callsign, _settings->cw_sstv.cw_repeats,
      _settings->cw_sstv.cw_wpm, _settings->cw_sstv.freq_mhz, _settings->cw_sstv.power_dbm,
      currentModeIndex(_settings), info.name, info.width, info.height, img);
    img.close();
  } else {
    LOG_E(TAG, "Impossible de relire l'image uploadée");
  }

  aprs_dispatcher.resume();

  LittleFS.remove(SSTV_IMAGE_PATH);
  _upload_complete = false;
  _upload_written_bytes = 0;

  LOG_I(TAG, "Transmission terminée");
}

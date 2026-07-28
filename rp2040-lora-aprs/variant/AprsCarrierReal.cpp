#include "AprsCarrierReal.h"
#include "aprs/SstvTransmitter.h"  // SSTV_MODE_LIST
#include "core/Log.h"
#include <RadioLib.h>
#include <target.h>

#define TAG "SSTV"

namespace {

// Table index->SSTVMode_t* : seule cette classe a besoin du mode RadioLib lui
// même (cf. aprs/SstvTransmitter.h pour la macro source, partagée avec la
// table nom<->index de SettingsRegistry et la table index->dimensions
// d'aprs/SstvTransmitter.cpp).
#define SSTV_MODE_PTR_ENTRY(idx, name, mode, w, h) &mode,
const SSTVMode_t* const SSTV_MODE_PTRS[SSTV_MODE_COUNT] = {
  SSTV_MODE_LIST(SSTV_MODE_PTR_ENTRY)
};
#undef SSTV_MODE_PTR_ENTRY

void sendCallsignMorse(MorseClient& morse, const char* callsign, uint8_t repeats) {
  for (uint8_t i = 0; i < repeats; i++) {
    morse.println(callsign);
  }
}

}  // namespace

void AprsCarrierReal::transmitCwSstv(const char* callsign, uint8_t cwRepeats, uint8_t cwWpm, float freqMhz,
                                     int8_t powerDbm, uint8_t modeIndex, const char* modeName,
                                     uint16_t width, uint16_t height, File& image) {
  LOG_I(TAG, "Début transmission (mode=%s, %dx%d)", modeName, width, height);

  // Réutilise switchToFsk() (déjà validée en réception WH65B réelle) plutôt
  // qu'une config FSK dédiée : seul le fait d'être en modem GFSK compte pour
  // transmitDirect()/directMode() (RadioLib SX126x::directMode() refuse le
  // mode direct hors GFSK), pas la fréquence WH65B qu'elle configure —
  // MorseClient/SSTVClient retendent la porteuse eux-mêmes sur la vraie
  // fréquence CW/SSTV à chaque symbole. La puissance, elle, n'est pas
  // retouchée par symbole : à réappliquer ici.
  bool ok = aprs_radio_hw.switchToFsk();
  if (ok) {
    _hw.setOutputPower(powerDbm);

    MorseClient morse(&_hw);
    morse.begin(freqMhz, cwWpm);
    sendCallsignMorse(morse, callsign, cwRepeats);

    SSTVClient sstv(&_hw);
    int16_t state = sstv.begin(freqMhz, *SSTV_MODE_PTRS[modeIndex]);
    if (state == RADIOLIB_ERR_NONE) {
      sstv.sendHeader();

      static uint32_t line[640];  // max largeur supportée (modes actuels: 320px)
      static uint8_t rgb[640 * 3];
      for (uint16_t y = 0; y < height; y++) {
        size_t n = image.read(rgb, (size_t)width * 3);
        if (n != (size_t)width * 3) {
          LOG_E(TAG, "Lecture image incomplète ligne %d (%u/%u octets)", y, (unsigned)n, (unsigned)(width * 3));
          break;
        }
        for (uint16_t x = 0; x < width; x++) {
          line[x] = ((uint32_t)rgb[x * 3] << 16) | ((uint32_t)rgb[x * 3 + 1] << 8) | (uint32_t)rgb[x * 3 + 2];
        }
        sstv.sendLine(line);
      }
    } else {
      LOG_E(TAG, "SSTVClient::begin: %d", state);
    }

    sendCallsignMorse(morse, callsign, cwRepeats);
    _hw.standby();
  }

  // switchToLora() repart des settings (radio.aprs.*), y compris la
  // puissance : pas de reapplication manuelle nécessaire ici.
  aprs_radio_hw.switchToLora();

  LOG_I(TAG, "Transmission terminée");
}

#include "AprsCarrierSim.h"
#include "core/Log.h"

#define TAG "SSTV"

void AprsCarrierSim::transmitCwSstv(const char* callsign, uint8_t cwRepeats, uint8_t cwWpm, float freqMhz,
                                    int8_t powerDbm, uint8_t modeIndex, const char* modeName,
                                    uint16_t width, uint16_t height, File& image) {
  (void)modeIndex;
  LOG_I(TAG, "[SIM] Début transmission (mode=%s, %dx%d)", modeName, width, height);
  LOG_I(TAG, "[SIM] CW indicatif '%s' x%d (%d WPM, %.3f MHz)", callsign, cwRepeats, cwWpm, freqMhz);
  LOG_I(TAG, "[SIM] SSTV début (%.3f MHz, %d dBm)", freqMhz, powerDbm);

  static uint8_t rgb[640 * 3];
  uint16_t lines_ok = 0;
  for (uint16_t y = 0; y < height; y++) {
    size_t n = image.read(rgb, (size_t)width * 3);
    if (n != (size_t)width * 3) {
      LOG_E(TAG, "[SIM] Lecture image incomplète ligne %d (%u/%u octets)", y, (unsigned)n, (unsigned)(width * 3));
      break;
    }
    lines_ok++;
  }
  LOG_I(TAG, "[SIM] SSTV: %d/%d lignes envoyées", lines_ok, height);

  LOG_I(TAG, "[SIM] CW indicatif '%s' x%d (fin)", callsign, cwRepeats);
}

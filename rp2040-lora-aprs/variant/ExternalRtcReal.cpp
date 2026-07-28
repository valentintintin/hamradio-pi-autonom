#include "ExternalRtcReal.h"
#include <RTClib.h>  // DateTime (adafruit/RTClib) — conversion epoch <-> tm uniquement
#include <time.h>

bool ExternalRtcReal::begin() {
  _present = _rtc.begin();
  return _present;
}

bool ExternalRtcReal::readTime(uint32_t* outUnixTime) {
  if (!_present || _rtc.getVLF()) {
    // VLF levé = alimentation coupée assez longtemps pour perdre l'heure :
    // ne pas faire confiance à cette lecture pour synchroniser autre chose.
    return false;
  }

  tm t{};
  _rtc.getTime(&t);
  DateTime dt(t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
  *outUnixTime = dt.unixtime();
  return true;
}

bool ExternalRtcReal::writeTime(uint32_t unixTime) {
  if (!_present) {
    return false;
  }

  DateTime dt(unixTime);
  tm t{};
  t.tm_year = dt.year() - 1900;
  t.tm_mon = dt.month() - 1;
  t.tm_mday = dt.day();
  t.tm_hour = dt.hour();
  t.tm_min = dt.minute();
  t.tm_sec = dt.second();
  t.tm_wday = dt.dayOfTheWeek();
  _rtc.setTime(&t);
  _rtc.clearVLF();
  return true;
}

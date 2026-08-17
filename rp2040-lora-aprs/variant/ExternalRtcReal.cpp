#include "ExternalRtcReal.h"
#include <RTClib.h>
#include <time.h>

bool ExternalRtcReal::begin() {
  if (!_bus->lock()) {
    return false;
  }
  _present = _rtc.begin();
  _bus->unlock();
  return _present;
}

bool ExternalRtcReal::readTime(uint32_t* outUnixTime) {
  if (!_present) {
    return false;
  }
  if (!_bus->lock()) {
    return false;
  }

  bool vlf = _rtc.getVLF();
  tm t{};
  if (!vlf) {
    _rtc.getTime(&t);
  }
  _bus->unlock();

  if (vlf) {
    // VLF levé = coupure d'alimentation assez longue pour perdre l'heure :
    // lecture non fiable, à ignorer.
    return false;
  }

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

  if (!_bus->lock()) {
    return false;
  }
  _rtc.setTime(&t);
  _rtc.clearVLF();
  _bus->unlock();
  return true;
}

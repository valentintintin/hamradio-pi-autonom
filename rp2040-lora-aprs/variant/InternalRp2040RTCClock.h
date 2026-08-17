#pragma once

#include <Mesh.h>
#include <RTClib.h>
#include <hardware/rtc.h>
#include <pico/util/datetime.h>

// Le RP2040 n'a pas de domaine d'alimentation RTC séparé battery-backed :
// cette horloge ne survit pas à un reset/watchdog, tout comme un compteur
// logiciel — mais reste un compteur matériel dédié, sans dérive liée à la
// charge FreeRTOS sur millis().
class InternalRp2040RTCClock : public mesh::RTCClock {
public:
  InternalRp2040RTCClock() {
    rtc_init();
    setCurrentTime(1715770351); // 15 mai 2024 20:50 UTC, écrasée dès qu'une heure valide est connue
  }

  uint32_t getCurrentTime() override {
    datetime_t dt;
    if (!rtc_get_datetime(&dt)) {
      return 0;
    }
    return DateTime(dt.year, dt.month, dt.day, dt.hour, dt.min, dt.sec).unixtime();
  }

  void setCurrentTime(uint32_t time) override {
    DateTime d(time);
    datetime_t dt;
    dt.year = d.year();
    dt.month = d.month();
    dt.day = d.day();
    dt.dotw = d.dayOfTheWeek();
    dt.hour = d.hour();
    dt.min = d.minute();
    dt.sec = d.second();
    rtc_set_datetime(&dt);
  }

  void tick() override {}
};

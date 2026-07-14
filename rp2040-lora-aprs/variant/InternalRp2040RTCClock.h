#pragma once

#include <Mesh.h>
#include <RTClib.h>
#include <hardware/rtc.h>
#include <pico/util/datetime.h>

// ============================================================================
// Horloge RTC interne du RP2040 (hardware/rtc.h) — utilisée en secours par
// AutoDiscoverRTCClock quand aucune puce RTC I2C n'est détectée.
//
// Ne survit pas à un reboot (le RP2040 n'a pas de domaine d'alimentation
// séparé type "RTC battery-backed" — tout l'état est perdu à chaque reset,
// watchdog compris), donc pas mieux qu'un compteur logiciel de ce point de
// vue. L'intérêt reste réel : compteur matériel dédié (pas de dérive liée à
// la charge FreeRTOS sur millis()), et c'est une ressource déjà présente sur
// la puce, sans coût supplémentaire.
// ============================================================================
class InternalRp2040RTCClock : public mesh::RTCClock {
public:
  InternalRp2040RTCClock() {
    rtc_init();
    setCurrentTime(1715770351); // 15 mai 2024, 20:50 UTC — écrasé dès qu'une heure valide est connue
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

  void tick() override {} // horloge matérielle, pas de mise à jour logicielle nécessaire
};

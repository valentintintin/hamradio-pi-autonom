#pragma once

#include "hal/chargers/MpptChargerHal.h"
#include "hal/eeprom/EventLogHistory.h"
#include "aprs/AprsEngine.h"

class MpptShutdownMonitor {
public:
  MpptShutdownMonitor(MpptChargerHal& mppt, AprsEngine& aprs, EventLogHistory& eventLog)
    : _mppt(&mppt), _aprs(&aprs), _event_log(&eventLog) {}

  // À appeler au démarrage de la tâche, pas au constructeur global : pinMode/attachInterrupt doivent
  // s'exécuter après l'init de l'Arduino core.
  void begin();

  void update();

private:
  MpptChargerHal* _mppt;
  AprsEngine* _aprs;
  EventLogHistory* _event_log;
  bool _alert_sent = false;

  static void onShutdownAlert();
};

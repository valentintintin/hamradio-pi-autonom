#pragma once

#include "hal/MpptChargerHal.h"
#include "hal/EventLogHistory.h"
#include "aprs/AprsEngine.h"

// ============================================================================
// MpptShutdownMonitor — alerte "extinction imminente" de la carte MPPT :
// GPIO (interruption, MPPT_ALERT_PIN) + bit I2C ALERT en secours, envoi d'un
// statut APRS une seule fois par épisode (cf. hal/MpptChargerHal.h).
// ============================================================================

class MpptShutdownMonitor {
public:
  MpptShutdownMonitor(MpptChargerHal& mppt, AprsEngine& aprs, EventLogHistory& eventLog)
    : _mppt(&mppt), _aprs(&aprs), _event_log(&eventLog) {}

  // Configure le GPIO et l'interruption — à appeler une fois au démarrage de
  // la tâche (pas au constructeur global : pinMode/attachInterrupt doivent
  // s'exécuter après l'init de l'Arduino core).
  void begin();

  // À appeler à chaque tick.
  void update();

private:
  MpptChargerHal* _mppt;
  AprsEngine* _aprs;
  EventLogHistory* _event_log;
  bool _alert_sent = false;

  static void onShutdownAlert();
};

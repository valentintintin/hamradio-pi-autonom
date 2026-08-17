#pragma once

#include "config/Settings.h"
#include "hal/relay/RelayHal.h"
#include "hal/eeprom/EventLogHistory.h"
#include <Timer.h>

// ============================================================================
// Relay — machine à états d'UN relais : coupure/reprise basse-tension
// (hystérésis + debounce) et réveil périodique, pilotées par les deux
// règles regroupées dans son propre Settings::Relay (cf. config/Settings.h).
// RelayHal reste l'unique source de vérité pour l'état ON/OFF réel (cf.
// hal/relay/RelayHal.h) — cette classe décide seulement QUAND l'actionner.
//
// Les deux comportements sont dans la même classe (plutôt que deux classes
// séparées) car ils s'arbitrent mutuellement sur la même ressource physique :
// un réveil périodique peut outrepasser une coupure basse-tension active
// (Settings::Relay::override_low_voltage), cf. updatePeriodic().
//
// Un troisième acteur, hors de cette classe, peut aussi outrepasser les deux :
// une commande utilisateur (CLI "relay N on/off", cf. CommandHandler::cmdRelay)
// arme RelayHal::isManualOverride() pour ce relais — update() suspend alors
// entièrement cutoff ET périodique jusqu'à "relay N auto" (contrôle total,
// décision produit ; volatile, reset au reboot).
//
// Ne connaît que son propre Settings::Relay (passé par référence à la
// construction), pas la Settings globale ni les autres relais — cf.
// task_energy.cpp, qui construit une instance par relais avec
// &settings.relay[i].
// ============================================================================

class Relay {
public:
  Relay(uint8_t index, Settings::Relay& config, RelayHal& relay, EventLogHistory& eventLog)
    : _index(index), _config(&config), _relay(&relay), _event_log(&eventLog) {}

  // À appeler à chaque tick avec la tension batterie courante (cf. task_energy.cpp)
  void update(float voltage_mv);

private:
  enum class CutoffState : uint8_t {
    Idle,               // règle désactivée, ou stable (rien en cours)
    ConfirmingCutoff,   // relais ON, sous-tension détectée, debounce coupure en cours
    ConfirmingRestore,  // relais OFF, tension revenue, debounce reprise en cours
    Overridden,         // supervision suspendue : réveil périodique force ON malgré une coupure
  };

  enum class PeriodicState : uint8_t {
    Disabled, // règle désactivée
    Waiting,  // OFF, en attente du prochain réveil (timer = interval_ms)
    Active,   // ON, fenêtre de réveil en cours (timer = on_duration_ms)
  };

  uint8_t _index;
  Settings::Relay* _config;
  RelayHal* _relay;
  EventLogHistory* _event_log;

  // Un seul timer pour le cutoff : à un instant donné, ce relais n'est
  // jamais à la fois "ON en train de confirmer une coupure" et "OFF en train
  // de confirmer une reprise" (mutuellement exclusifs, cf. updateCutoff).
  CutoffState _cutoff_state = CutoffState::Idle;
  Timer _cutoff_timer;

  PeriodicState _periodic_state = PeriodicState::Disabled;
  Timer _periodic_timer;

  void updateCutoff(float voltage_mv);
  void updatePeriodic(float voltage_mv);

  // Ce relais serait-il coupé par sa règle de tension, vue instantanée sans
  // debounce — utilisé par updatePeriodic pour décider si un réveil override
  // réellement une coupure qui serait active.
  bool wouldBeCut(float voltage_mv) const;
};

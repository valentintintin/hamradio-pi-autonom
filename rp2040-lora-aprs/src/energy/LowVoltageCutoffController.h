#pragma once

#include "config/Settings.h"
#include "hal/RelayHal.h"
#include "hal/EventLogHistory.h"
#include <Timer.h>

// ============================================================================
// LowVoltageCutoffController — coupure/reprise basse-tension par relais avec
// hystérésis réelle (Schmitt trigger) et confirmation temporelle (debounce).
// cf. Settings.h: RelayCutoffRule (min/restore/debounce par relais).
// ============================================================================

class LowVoltageCutoffController {
public:
  LowVoltageCutoffController(Settings& settings, RelayHal& relay, EventLogHistory& eventLog)
    : _settings(&settings), _relay(&relay), _event_log(&eventLog) {}

  // À appeler à chaque tick avec la tension batterie courante (cf.
  // task_energy.cpp). Ne fait rien si low_voltage_cutoff_enabled est faux ou
  // si aucune tension n'est disponible.
  void update(float voltage_mv, bool have_voltage);

  // Suspend/réactive la coupure pour ce relais pendant qu'une fenêtre de
  // réveil périodique en override est active (cf. RelayPeriodicController).
  void setOverrideActive(uint8_t relay_idx, bool active) {
    _override_active[relay_idx] = active;
  }

  // Le relais serait-il actuellement coupé par sa règle (si elle existe),
  // vue instantanée sans debounce — utilisé par RelayPeriodicController pour
  // décider si un réveil override réellement une coupure active.
  bool wouldBeCut(uint8_t relay_idx, float voltage_mv, bool have_voltage) const;

private:
  Settings* _settings;
  RelayHal* _relay;
  EventLogHistory* _event_log;

  bool _override_active[RELAY_COUNT] = {false};

  // Un Timer par règle et par sens (coupure/reprise) : la condition doit
  // être vraie sans interruption pendant debounce_ms avant que l'action ne
  // soit prise. *_confirm_armed distingue "jamais vu la condition" de "timer
  // expiré" (un Timer par défaut est déjà "expiré").
  Timer _cutoff_confirm_timer[RELAY_CUTOFF_COUNT];
  bool _cutoff_confirm_armed[RELAY_CUTOFF_COUNT] = {false};
  Timer _restore_confirm_timer[RELAY_CUTOFF_COUNT];
  bool _restore_confirm_armed[RELAY_CUTOFF_COUNT] = {false};

  const RelayCutoffRule* findRule(uint8_t relay_idx) const;
};

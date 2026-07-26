#pragma once

#include "config/Settings.h"
#include "hal/RelayHal.h"
#include "LowVoltageCutoffController.h"
#include <Timer.h>

// ============================================================================
// RelayPeriodicController — réveil périodique par relais : allume pendant
// on_duration_ms toutes les interval_ms, indépendamment de la coupure
// basse-tension (avec possibilité d'override, cf. Settings.h:
// RelayPeriodicRule).
// ============================================================================

class RelayPeriodicController {
public:
  RelayPeriodicController(Settings& settings, RelayHal& relay, LowVoltageCutoffController& cutoff)
    : _settings(&settings), _relay(&relay), _cutoff(&cutoff) {}

  void update(float voltage_mv, bool have_voltage);

private:
  Settings* _settings;
  RelayHal* _relay;
  LowVoltageCutoffController* _cutoff;

  // Un seul Timer par relais, reconfiguré à chaque transition : arme
  // interval_ms en attente d'allumage, puis on_duration_ms une fois allumé.
  Timer _timer[RELAY_COUNT];
  bool _armed[RELAY_COUNT] = {false};  // false tant que la règle n'a jamais été (re)armée
  bool _active[RELAY_COUNT] = {false};
};

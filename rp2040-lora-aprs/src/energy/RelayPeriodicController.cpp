#include "RelayPeriodicController.h"
#include "core/Log.h"

#define TAG "ENERGY"

void RelayPeriodicController::update(float voltage_mv, bool have_voltage) {
  for (uint8_t idx = 0; idx < RELAY_COUNT; idx++) {
    const RelayPeriodicRule& rule = _settings->relay_periodic[idx];

    if (!rule.enabled) {
      _active[idx] = false;
      _armed[idx] = false; // réarmera proprement si réactivée plus tard
      _cutoff->setOverrideActive(idx, false);
      continue;
    }

    if (!_armed[idx]) {
      // Règle tout juste activée (ou premier passage) : démarre l'attente
      _timer[idx].setInterval(rule.interval_ms, true);
      _armed[idx] = true;
    }

    if (!_timer[idx].hasExpired()) {
      continue;
    }

    if (_active[idx]) {
      // Fin de fenêtre ON
      _active[idx] = false;
      _cutoff->setOverrideActive(idx, false);
      _relay->setState(idx, false);
      _timer[idx].setInterval(rule.interval_ms, true);
      LOG_T(TAG, "Relais %u : fin fenêtre réveil périodique", idx + 1);
      continue;
    }

    // Attente expirée : c'est l'heure de la fenêtre ON
    bool would_be_cut = _cutoff->wouldBeCut(idx, voltage_mv, have_voltage);

    if (would_be_cut && !rule.override_low_voltage) {
      LOG_T(TAG, "Relais %u : réveil périodique sauté (sous-tension)", idx + 1);
      _timer[idx].setInterval(rule.interval_ms, true); // retente au prochain cycle
      continue;
    }

    _active[idx] = true;
    _cutoff->setOverrideActive(idx, would_be_cut); // true seulement si on outrepasse une coupure active
    _relay->setState(idx, true);
    _timer[idx].setInterval(rule.on_duration_ms, true);
    LOG_T(TAG, "Relais %u : début fenêtre réveil périodique (%lums)%s",
      idx + 1, (unsigned long)rule.on_duration_ms, would_be_cut ? " [override sous-tension]" : "");
  }
}

#include "LowVoltageCutoffController.h"
#include "core/Log.h"

#define TAG "ENERGY"

const RelayCutoffRule* LowVoltageCutoffController::findRule(uint8_t relay_idx) const {
  for (uint8_t i = 0; i < RELAY_CUTOFF_COUNT; i++) {
    const RelayCutoffRule& rule = _settings->relay_cutoff[i];
    if (rule.relay_number != 0 && (rule.relay_number - 1) == relay_idx) {
      return &rule;
    }
  }
  return nullptr;
}

bool LowVoltageCutoffController::wouldBeCut(uint8_t relay_idx, float voltage_mv, bool have_voltage) const {
  const RelayCutoffRule* rule = findRule(relay_idx);
  return rule && have_voltage && voltage_mv < rule->min_voltage_mv;
}

void LowVoltageCutoffController::update(float voltage_mv, bool have_voltage) {
  if (!_settings->energy.low_voltage_cutoff_enabled || !have_voltage) {
    return;
  }

  for (uint8_t i = 0; i < RELAY_CUTOFF_COUNT; i++) {
    const RelayCutoffRule& rule = _settings->relay_cutoff[i];
    if (rule.relay_number == 0) {
      _cutoff_confirm_armed[i] = false;
      _restore_confirm_armed[i] = false;
      continue; // règle désactivée
    }
    uint8_t idx = rule.relay_number - 1;

    if (_override_active[idx]) {
      // Fenêtre de réveil périodique en cours avec override : ne pas couper,
      // et repartir de zéro sur la confirmation une fois la fenêtre finie
      // plutôt que d'agir sur un timer resté armé pendant tout ce temps.
      _cutoff_confirm_armed[i] = false;
      continue;
    }

    // Hystérésis réelle (Schmitt trigger) : l'état ne dépend que de l'état
    // actuel du relais et de la tension, pas de qui l'a mis dans cet état —
    // un relais remonté manuellement en CLI pendant que la tension est basse
    // sera donc lui aussi recoupé, et inversement un relais éteint
    // manuellement sera rallumé si la tension est déjà au-dessus du seuil de
    // reprise. C'est le comportement attendu d'une hystérésis.
    //
    // restore_voltage_mv doit être strictement supérieur à min_voltage_mv
    // pour éviter le pompage (bande de garde nulle ou négative) ; sinon la
    // reconnexion auto est simplement désactivée pour cette règle (coupure
    // seule, reconnexion manuelle via CLI "relay.N.state").
    bool restore_valid = rule.restore_voltage_mv > rule.min_voltage_mv;
    bool low_condition = _settings->relay[idx].state && voltage_mv < rule.min_voltage_mv;
    bool high_condition = !_settings->relay[idx].state && restore_valid && voltage_mv >= rule.restore_voltage_mv;

    if (low_condition) {
      if (!_cutoff_confirm_armed[i]) {
        _cutoff_confirm_timer[i].setInterval(rule.debounce_ms, true);
        _cutoff_confirm_armed[i] = true;
      }
      if (_cutoff_confirm_timer[i].hasExpired()) {
        LOG_W(TAG, "Sous-tension confirmée (%.0fmV < %umV depuis %lums) : coupure relais %u",
          voltage_mv, rule.min_voltage_mv, (unsigned long)rule.debounce_ms, rule.relay_number);
        _relay->setState(idx, false);
        _cutoff_confirm_armed[i] = false;
      }
    } else {
      _cutoff_confirm_armed[i] = false; // condition retombée avant confirmation : on annule
    }

    if (high_condition) {
      if (!_restore_confirm_armed[i]) {
        _restore_confirm_timer[i].setInterval(rule.debounce_ms, true);
        _restore_confirm_armed[i] = true;
      }
      if (_restore_confirm_timer[i].hasExpired()) {
        LOG_I(TAG, "Tension rétablie confirmée (%.0fmV >= %umV depuis %lums) : reconnexion relais %u",
          voltage_mv, rule.restore_voltage_mv, (unsigned long)rule.debounce_ms, rule.relay_number);
        _relay->setState(idx, true);
        _restore_confirm_armed[i] = false;
      }
    } else {
      _restore_confirm_armed[i] = false;
    }
  }
}

#include "hal/relay//Relay.h"
#include "core/Log.h"

#define TAG "RELAY"

bool Relay::wouldBeCut(float voltage_mv) const {
  return _config->cutoff_enabled && voltage_mv < _config->min_voltage_mv;
}

void Relay::update(float voltage_mv) {
  // Cutoff avant periodic (comme historiquement) : periodic peut décider
  // cette tick de passer en Overridden, ce que cutoff ne verra qu'au tick
  // suivant — un tick de latence sans conséquence (cf. Relay.h).
  updateCutoff(voltage_mv);
  updatePeriodic(voltage_mv);
}

void Relay::updateCutoff(float voltage_mv) {
  const Settings::Relay& cfg = *_config;
  CutoffState& state = _cutoff_state;

  if (!cfg.cutoff_enabled) {
    state = CutoffState::Idle;
    return;
  }

  if (state == CutoffState::Overridden) {
    // Reste suspendu tant que le réveil périodique n'a pas explicitement
    // levé l'override (cf. updatePeriodic, seul endroit qui en sort).
    return;
  }

  // restore_voltage_mv == 0 est une valeur documentée ("pas de reconnexion
  // auto"), pas une erreur de config : ne doit pas empêcher la coupure.
  bool auto_restore = cfg.restore_voltage_mv != 0;
  if (auto_restore && cfg.restore_voltage_mv <= cfg.min_voltage_mv) {
    LOG_W(TAG, "Relais %u: restore_voltage_mv <= min_voltage_mv (%u <= %u), reprise auto ignorée",
      _index + 1, cfg.restore_voltage_mv, cfg.min_voltage_mv);
    auto_restore = false;
  }

  if (_relay->getState(_index)) {
    bool under_voltage = voltage_mv < cfg.min_voltage_mv;
    if (!under_voltage) {
      if (state == CutoffState::ConfirmingCutoff) {
        LOG_T(TAG, "La tension est OK pour %u (%.0fmV >= %umV)", _index + 1, voltage_mv, cfg.min_voltage_mv);
      }
      state = CutoffState::Idle;
      return;
    }
    if (state != CutoffState::ConfirmingCutoff) {
      LOG_D(TAG, "Sous tension pour %u. Début timer tension basse pour %lums", _index + 1, (unsigned long)cfg.debounce_ms);
      _cutoff_timer.setInterval(cfg.debounce_ms, true);
      state = CutoffState::ConfirmingCutoff;
      return;
    }
    if (_cutoff_timer.hasExpired()) {
      LOG_W(TAG, "Sous-tension confirmée (%.0fmV < %umV depuis %lums) : coupure relais %u",
        voltage_mv, cfg.min_voltage_mv, (unsigned long)cfg.debounce_ms, _index + 1);
      _event_log->log(EVENT_LOW_VOLTAGE_CUTOFF, _index + 1, (int32_t)voltage_mv);
      _relay->setState(_index, false);
      state = CutoffState::Idle;
    }
    return;
  }

  // Relais OFF
  bool restore_ready = auto_restore && voltage_mv >= cfg.restore_voltage_mv;
  if (!restore_ready) {
    if (state == CutoffState::ConfirmingRestore) {
      LOG_T(TAG, "Relais %u: tension pas encore rétablie, confirmation annulée", _index + 1);
    }
    state = CutoffState::Idle;
    return;
  }
  if (state != CutoffState::ConfirmingRestore) {
    LOG_T(TAG, "Sur tension pour %u. Début timer tension haute pour %lums", _index + 1, (unsigned long)cfg.debounce_ms);
    _cutoff_timer.setInterval(cfg.debounce_ms, true);
    state = CutoffState::ConfirmingRestore;
    return;
  }
  if (_cutoff_timer.hasExpired()) {
    LOG_I(TAG, "Tension rétablie confirmée (%.0fmV >= %umV depuis %lums) : reconnexion relais %u",
      voltage_mv, cfg.restore_voltage_mv, (unsigned long)cfg.debounce_ms, _index + 1);
    _event_log->log(EVENT_LOW_VOLTAGE_RESTORE, _index + 1, (int32_t)voltage_mv);
    _relay->setState(_index, true);
    state = CutoffState::Idle;
  }
}

void Relay::updatePeriodic(float voltage_mv) {
  const Settings::Relay& cfg = *_config;
  PeriodicState& state = _periodic_state;

  if (!cfg.periodic_enabled) {
    if (state == PeriodicState::Active && _cutoff_state == CutoffState::Overridden) {
      _cutoff_state = CutoffState::Idle; // ne pas laisser cutoff bloqué en Overridden
      LOG_T(TAG, "Relais %u : désactivation", _index + 1);
    }
    state = PeriodicState::Disabled;
    return;
  }

  if (state == PeriodicState::Disabled) {
    // Règle tout juste (ré)activée : démarre l'attente
    _periodic_timer.setInterval(cfg.interval_ms, true);
    state = PeriodicState::Waiting;
    LOG_I(TAG, "Relais %u : début timer pour réveil périodique", _index + 1, (unsigned long)cfg.interval_ms);
  }

  if (!_periodic_timer.hasExpired()) {
    return;
  }

  if (state == PeriodicState::Active) {
    // Fin de fenêtre ON
    if (_cutoff_state == CutoffState::Overridden) {
      _cutoff_state = CutoffState::Idle;
      LOG_W(TAG, "Relais %u : fin fenêtre réveil périodique met surchargé donc on laisse à allumé", _index + 1);
    }
    _relay->setState(_index, false);
    _periodic_timer.setInterval(cfg.interval_ms, true);
    state = PeriodicState::Waiting;
    LOG_I(TAG, "Relais %u : fin fenêtre réveil périodique (%lums)%s", _index + 1, (unsigned long)cfg.interval_ms);
    return;
  }

  // Attente expirée : c'est l'heure de la fenêtre ON
  bool would_be_cut = wouldBeCut(voltage_mv);

  if (would_be_cut && !cfg.override_low_voltage) {
    LOG_T(TAG, "Relais %u : réveil périodique sauté (sous-tension)", _index + 1);
    _periodic_timer.setInterval(cfg.interval_ms, true); // retente au prochain cycle
    return;
  }

  if (would_be_cut) {
    _cutoff_state = CutoffState::Overridden; // outrepasse une coupure active
  }
  _relay->setState(_index, true);
  _periodic_timer.setInterval(cfg.on_duration_ms, true);
  state = PeriodicState::Active;
  LOG_I(TAG, "Relais %u : début fenêtre réveil périodique (%lums)%s",
    _index + 1, (unsigned long)cfg.on_duration_ms, would_be_cut ? " [override sous-tension]" : "");
}

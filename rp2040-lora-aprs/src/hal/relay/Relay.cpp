#include "hal/relay//Relay.h"
#include "core/Log.h"

#define TAG "RELAY"

void Relay::update(float voltage_mv) {
  if (_manual_override) {
    if (_manual_override_timeout_ms != 0 && _manual_override_timer.hasExpired()) {
      LOG_W(TAG, "Relais %u : override manuel expiré après %lums, retour à l'automatisme",
        _index + 1, (unsigned long)_manual_override_timeout_ms);
      _event_log->log(EVENT_RELAY_MANUAL_CLEARED, _index + 1, 1 /* data1=1 : expiration auto, pas "relay N auto" */);
      setManualOverride(false);
      // Pas de "return" : on continue vers les règles ci-dessous sans attendre le tick suivant.
    } else {
      return;
    }
  }

  bool relayState = getState();
  bool cutoff = updateCutoff(voltage_mv, relayState);
  bool periodic = updatePeriodic(relayState);

  const Settings::Relay& cfg = *_config;
  bool newState;
  if (cfg.periodic_enabled && cfg.override_low_voltage) {
    newState = periodic;
  } else if (cfg.cutoff_enabled && cfg.periodic_enabled) {
    newState = cutoff && periodic; // règle 1 = garde-fou, coupe même pendant une fenêtre de règle périodiques
  } else if (cfg.cutoff_enabled) {
    newState = cutoff;
  } else if (cfg.periodic_enabled) {
    newState = periodic;
  } else {
    newState = relayState;
  }

  if (newState != relayState) {
    _relay_hal->setState(_index, newState);
  }
}

bool Relay::setManualState(bool on, uint32_t timeout_ms) {
  bool ok = _relay_hal->setState(_index, on);
  setManualOverride(true, timeout_ms);
  return ok;
}

void Relay::setManualOverride(bool override, uint32_t timeout_ms) {
  _manual_override = override;
  _manual_override_timeout_ms = override ? timeout_ms : 0;
  if (override && timeout_ms != 0) {
    _manual_override_timer.setInterval(timeout_ms, true);
  }
}

bool Relay::updateCutoff(float voltage_mv, bool relayState) {
  const Settings::Relay& cfg = *_config;

  if (!cfg.cutoff_enabled) {
    _cutoff_pending_valid = false;
    return relayState;
  }

  // Tension à 0 = pas encore de relevé capteur valide, ne pas agir dessus.
  if (voltage_mv <= 0) {
    return relayState;
  }

  bool desired = (voltage_mv >= cfg.min_voltage_mv) && (voltage_mv <= cfg.restore_voltage_mv);

  if (desired == relayState) {
    if (_cutoff_pending_valid) {
      LOG_T(TAG, "Relais %u : tension revenue du bon côté, confirmation annulée", _index + 1);
    }
    _cutoff_pending_valid = false;
    return relayState;
  }

  if (!_cutoff_pending_valid || _cutoff_pending_state != desired) {
    LOG_D(TAG, "Relais %u : tension %.0fmV hors bande souhaitée (%s), début debounce %lums",
      _index + 1, voltage_mv, desired ? "veut ON" : "veut OFF", (unsigned long)cfg.debounce_ms);
    _cutoff_pending_valid = true;
    _cutoff_pending_state = desired;
    _cutoff_timer.setInterval(cfg.debounce_ms, true);
    return relayState;
  }

  if (!_cutoff_timer.hasExpired()) {
    return relayState;
  }

  _cutoff_pending_valid = false;
  if (desired) {
    LOG_I(TAG, "Tension rétablie confirmée (%.0fmV dans [%u, %u]mV depuis %lums) : reconnexion relais %u",
      voltage_mv, cfg.min_voltage_mv, cfg.restore_voltage_mv, (unsigned long)cfg.debounce_ms, _index + 1);
    _event_log->log(EVENT_LOW_VOLTAGE_RESTORE, _index + 1, (int32_t)voltage_mv);
  } else {
    LOG_W(TAG, "Sous/sur-tension confirmée (%.0fmV hors [%u, %u]mV depuis %lums) : coupure relais %u",
      voltage_mv, cfg.min_voltage_mv, cfg.restore_voltage_mv, (unsigned long)cfg.debounce_ms, _index + 1);
    _event_log->log(EVENT_LOW_VOLTAGE_CUTOFF, _index + 1, (int32_t)voltage_mv);
  }
  return desired;
}

bool Relay::updatePeriodic(bool relayState) {
  const Settings::Relay& cfg = *_config;

  if (!cfg.periodic_enabled) {
    return relayState;
  }

  if (!_periodic_initialized) {
    _periodic_initialized = true;
    _periodic_phase_on = true;
    _periodic_timer.setInterval(cfg.on_duration_ms, true);
    LOG_I(TAG, "Relais %u : cycle périodique démarré (ON %lums / OFF %lums)",
      _index + 1, (unsigned long)cfg.on_duration_ms, (unsigned long)cfg.interval_ms);
  } else if (_periodic_timer.hasExpired()) {
    _periodic_phase_on = !_periodic_phase_on;
    _periodic_timer.setInterval(_periodic_phase_on ? cfg.on_duration_ms : cfg.interval_ms, true);
    LOG_T(TAG, "Relais %u : cycle périodique -> %s", _index + 1, _periodic_phase_on ? "ON" : "OFF");
  }

  return _periodic_phase_on;
}

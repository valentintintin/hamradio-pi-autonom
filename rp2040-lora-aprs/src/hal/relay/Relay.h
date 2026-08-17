#pragma once

#include "config/Settings.h"
#include "hal/relay/RelayHal.h"
#include "hal/eeprom/EventLogHistory.h"
#include <Timer.h>

class Relay {
public:
  Relay(uint8_t index, Settings::Relay& config, RelayHal& relay, EventLogHistory& eventLog)
    : _index(index), _config(&config), _relay_hal(&relay), _event_log(&eventLog) {}

  void update(float voltage_mv);

  // timeout_ms = 0 : jamais d'expiration. Retourne false si le TCA9555 n'a
  // pas répondu (l'override est quand même armé).
  bool setManualState(bool on, uint32_t timeout_ms);
  void setManualOverride(bool override, uint32_t timeout_ms = 0);

  bool isManualOverride() const { return _manual_override; }
  bool getState() const { return _relay_hal->getState(_index); }

private:
  uint8_t _index;
  Settings::Relay* _config;
  RelayHal* _relay_hal;
  EventLogHistory* _event_log;

  bool _cutoff_pending_valid = false;
  bool _cutoff_pending_state = false;
  Timer _cutoff_timer;

  // _periodic_initialized (pas Timer::isPaused()) : un Timer par défaut a
  // déjà hasExpired()==true à la construction, donc isPaused() ne détecte
  // pas "jamais armé". Gelé (pas remis à false) quand periodic_enabled
  // repasse à false, pour reprendre le cycle où il en était.
  bool _periodic_initialized = false;
  bool _periodic_phase_on = true;
  Timer _periodic_timer;

  bool _manual_override = false;
  uint32_t _manual_override_timeout_ms = 0; // 0 = jamais
  Timer _manual_override_timer;

  bool updateCutoff(float voltage_mv, bool relayState);
  bool updatePeriodic(bool relayState);
};

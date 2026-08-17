#pragma once

// Une trame APRS RF n'authentifie pas son callsign source (trivialement
// usurpable) : les commandes qui modifient l'état exigent le mot de passe
// admin en premier mot du message (cf. onAprsMessageReceived).

#include "AprsEngine.h"
#include "hal/Telemetry.h"
#include "cli/CommandHandler.h"
#include "config/Settings.h"
#include "hal/relay/Relay.h"

class AprsEventHandler : public AprsEventCallback {
public:
  AprsEventHandler(AprsEngine& engine, CommandHandler& commandHandler,
                   TelemetryData& telemetry, Settings& settings, Relay* relays);

  void onAprsMessageReceived(const char* from, const char* message) override;
  void fillTelemetryData(aprs::Telemetry& telemetry) override;
  void fillWeatherData(aprs::Weather& weather) override;
  void fillStatusText(char* buf, size_t len) override;

private:
  AprsEngine* _engine;
  CommandHandler* _cmd;
  TelemetryData* _telemetry;
  Settings* _settings;
  Relay* _relays;
};

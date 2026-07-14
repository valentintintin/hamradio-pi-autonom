#pragma once

// ============================================================================
// AprsEventHandler — relie AprsEngine à la télémétrie et au CLI
//
// - fillTelemetryData / fillWeatherData : alimentent les beacons périodiques
//   avec les données courantes (INA3221, MPPT, BME280, WH65B, Victron).
// - fillStatusText : résume l'état courant de la station (batterie, MPPT...)
//   pour un statut APRS qui reflète l'état réel plutôt qu'un texte figé.
// - onAprsMessageReceived : un message APRS adressé à nous est exécuté comme
//   une commande CLI (CommandHandler) et la réponse est renvoyée en message.
//   Contrairement au port série, une trame APRS RF n'authentifie pas son
//   callsign source (trivialement usurpable) : les commandes qui modifient
//   l'état (set/save/reboot/defaults) exigent donc le mot de passe admin en
//   premier mot du message.
// ============================================================================

#include "AprsEngine.h"
#include "hal/Telemetry.h"
#include "config/CommandHandler.h"
#include "config/Settings.h"

class AprsEventHandler : public AprsEventCallback {
public:
  AprsEventHandler(AprsEngine& engine, CommandHandler& commandHandler,
                   TelemetryData& telemetry, Settings& settings);

  void onAprsMessageReceived(const char* from, const char* message) override;
  void fillTelemetryData(aprs::Telemetry& telemetry) override;
  void fillWeatherData(aprs::Weather& weather) override;
  void fillStatusText(char* buf, size_t len) override;

private:
  AprsEngine* _engine;
  CommandHandler* _cmd;
  TelemetryData* _telemetry;
  Settings* _settings;

  static bool isPrivilegedCommand(const char* cmd);
};

#include "AprsEventHandler.h"
#include "core/Log.h"
#include <string.h>
#include <stdio.h>

#define TAG "APRS-EVT"

AprsEventHandler::AprsEventHandler(AprsEngine& engine, CommandHandler& commandHandler,
                                   TelemetryData& telemetry, Settings& settings)
  : _engine(&engine), _cmd(&commandHandler), _telemetry(&telemetry), _settings(&settings)
{
}

// ============================================================================
// Message adressé à nous → exécuté comme une commande CLI, réponse par message
// ============================================================================
void AprsEventHandler::onAprsMessageReceived(const char* from, const char* message) {
  char cmd_buf[128];
  strncpy(cmd_buf, message, sizeof(cmd_buf) - 1);
  cmd_buf[sizeof(cmd_buf) - 1] = '\0';

  char* cmd = cmd_buf;
  while (*cmd == ' ') {
    cmd++;
  }

  size_t pwd_len = strlen(_settings->system.admin_password);
  bool authenticated = pwd_len > 0 &&
    strncmp(cmd, _settings->system.admin_password, pwd_len) == 0 &&
    (cmd[pwd_len] == ' ' || cmd[pwd_len] == '\0');

  if (authenticated) {
    cmd += pwd_len;
    while (*cmd == ' ') {
      cmd++;
    }
  } else if (_cmd->isPrivilegedCommand(cmd)) {
    LOG_W(TAG, "MSG de %s refusé (commande privilégiée sans mot de passe)", from);
    _engine->sendMessage(from, "Unauthorized");
    return;
  }

  char reply[100];
  reply[0] = '\0';
  bool recognized = _cmd->execute(cmd, reply, sizeof(reply));
  if (!recognized) {
    snprintf(reply, sizeof(reply), "Unknown: %.30s", cmd);
  }

  if (reply[0]) {
    _engine->sendMessage(from, reply);
  }
}

// ============================================================================
// Télémétrie — 5 canaux analogiques + quelques flags d'état
// ============================================================================
void AprsEventHandler::fillTelemetryData(aprs::Telemetry& telemetry) {
  auto setAnalog = [](aprs::AnalogChannel& ch, const char* name, const char* unit, double value) {
    strncpy(ch.name, name, sizeof(ch.name) - 1);
    strncpy(ch.unit, unit, sizeof(ch.unit) - 1);
    ch.value = value;
  };
  auto setBool = [](aprs::BooleanChannel& ch, const char* name, bool value) {
    strncpy(ch.name, name, sizeof(ch.name) - 1);
    ch.value = value;
  };

  setAnalog(telemetry.analog[0], "BattV", "V",  _telemetry->battery_mppt.voltage_mv / 1000.0);
  setAnalog(telemetry.analog[1], "SolV",  "V",  _telemetry->solar_mppt.voltage_mv / 1000.0);
  setAnalog(telemetry.analog[2], "BattI", "mA", _telemetry->battery_mppt.current_ma);
  setAnalog(telemetry.analog[3], "SolI",  "mA", _telemetry->solar_mppt.current_ma);

  setBool(telemetry.boolean[0], "WiFi", _settings->relay[0].state);
  setBool(telemetry.boolean[1], "Cam", _settings->relay[1].state);
  setBool(telemetry.boolean[2], "Pi", _settings->relay[2].state);
  // setBool(telemetry.boolean[3], "", _settings->relay[3].state);

  strncpy(telemetry.projectName, "LoRa APRS + Meshcore", sizeof(telemetry.projectName) - 1);
}

// ============================================================================
// Météo — BME280 (toujours dispo si initialisé) + WH65B (station extérieure,
// si entendue récemment)
// ============================================================================
void AprsEventHandler::fillWeatherData(aprs::Weather& weather) {
  weather.useTemperature = true;
  weather.temperatureFahrenheit = (int16_t)(_telemetry->weather_outside.base.temperature_c * 9.0 / 5.0 + 32.0);

  weather.useHumidity = true;
  weather.humidity = (uint8_t)_telemetry->weather_outside.base.humidity;

  weather.usePressure = true;
  weather.pressure = (uint16_t)(_telemetry->weather_inside.pressure_hpa * 10.0);

  if (_telemetry->weather_outside.is_valid) {
    weather.useWindDirection = true;
    weather.windDirectionDegrees = (uint16_t)_telemetry->weather_outside.wind_dir_deg;

    weather.useWindSpeed = true;
    weather.windSpeedMph = (uint16_t)(_telemetry->weather_outside.wind_avg_ms * 2.23694f);

    weather.useGustSpeed = true;
    weather.gustSpeedMph = (uint16_t)(_telemetry->weather_outside.wind_max_ms * 2.23694f);

    // Le WH65B ne fournit qu'un cumul de pluie, pas de fenêtre glissante 24h ;
    // on le reporte tel quel dans le champ "rain 24h" faute de mieux.
    weather.useRain24Hour = true;
    weather.rain24HourHundredthsOfAnInch = (uint16_t)(_telemetry->weather_outside.rain_mm * 3.93701f);
  }
}

// ============================================================================
// Statut — reflète l'état réel de la station plutôt qu'un texte figé
// ============================================================================
void AprsEventHandler::fillStatusText(char* buf, size_t len) {
  const char* state = "OK";

  // TODO gérer le statut ?

  snprintf(buf, len, "%s Bat=%.1fV Sol=%.1fV SOC=%.0f%% Up=%lus",
    state,
    _telemetry->battery_mppt.voltage_mv / 1000.0f,
    _telemetry->solar_mppt.voltage_mv / 1000.0f,
    _telemetry->victron_soc,
    _telemetry->uptime_s);
}

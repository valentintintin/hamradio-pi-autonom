#include "CommandHandler.h"
#include "core/Log.h"
#include "core/Version.h"
#include "core/LockGuard.h"
#include "core/StringPrint.h"
#include <target.h>
#include <RTClib.h>
#include <cstring>
#include <cstdio>

extern LogLevel g_log_level;

// ============================================================================
// Execute — dispatch sur la commande
//
// Le verrou couvre tous les points de sortie (nombreux "return" ci-dessous).
// Récursif (partagé avec AprsEngine, cf. AprsEngine::getMutex()) car un
// message APRS reçu tient déjà ce verrou (dans AprsEngine::onAprsPacketReceived)
// quand il appelle execute() via AprsEventHandler, et execute() lui-même peut
// rappeler AprsEngine (commandes "beacon"/"wx"/"send aprs").
// ============================================================================
bool CommandHandler::execute(const char* input, Print& out, bool isLocal) {
  RecursiveLockGuard lock(_mutex);

  // Copie locale pour tokeniser (assez grand pour "send aprs <contenu>",
  // le contenu APRS pouvant aller jusqu'à ~200 caractères)
  char buf[220];
  strncpy(buf, input, sizeof(buf) - 1);
  buf[sizeof(buf) - 1] = '\0';

  // Trim leading spaces
  char* cmd = buf;
  while (*cmd == ' ') {
    cmd++;
  }

  if (strncmp(cmd, "get ", 4) == 0) {
    cmdGet(cmd + 4, out);
    return true;
  }

  if (strncmp(cmd, "set ", 4) == 0) {
    // Trouver la clé et la valeur
    char* key = cmd + 4;
    while (*key == ' ') {
      key++;
    }
    char* value = strchr(key, ' ');
    if (value) {
      *value = '\0';
      value++;
      while (*value == ' ') {
        value++;
      }
      cmdSet(key, value, out);
    } else {
      out.println(F("Usage: set <key> <value>"));
    }
    return true;
  }

  // "clockdate" (pas "clock"/"time") : évite toute collision avec les
  // commandes CLI MeshCore du même nom (lecture "clock", réglage "time ").
  if (strncmp(cmd, "clockdate ", 10) == 0) {
    cmdSetClockDate(cmd + 10, out);
    return true;
  }

  if (strcmp(cmd, "list") == 0) {
    cmdList(out);
    return true;
  }

  if (strcmp(cmd, "save") == 0) {
    cmdSave(out);
    return true;
  }

  if (strcmp(cmd, "status") == 0) {
    cmdStatus(out);
    return true;
  }

  if (strcmp(cmd, "version") == 0) {
    cmdVersion(out);
    return true;
  }

  if (strcmp(cmd, "uptime") == 0) {
    out.printf("Uptime: %lu s\n", _telemetry->uptime_s);
    return true;
  }

  if (strcmp(cmd, "freemem") == 0) {
    out.printf("Free heap: %u bytes\n", (unsigned)rp2040.getFreeHeap());
    return true;
  }

  if (strcmp(cmd, "beacon") == 0) {
    out.println(_aprs->sendPosition(_settings->aprs.comment) ? F("Position envoyée") : F("Échec envoi position"));
    return true;
  }

  if (strcmp(cmd, "wx") == 0) {
    out.println(_aprs->sendWeather() ? F("Météo envoyée") : F("Échec envoi météo"));
    return true;
  }

  if (strncmp(cmd, "send aprs ", 10) == 0) {
    cmdSendAprs(cmd + 10, out);
    return true;
  }

  if (strncmp(cmd, "history", 7) == 0) {
    const char* args = (cmd[7] == ' ') ? cmd + 8 : "";
    cmdHistory(args, out);
    return true;
  }

  if (strcmp(cmd, "reboot") == 0) {
    out.println(F("Redémarrage..."));
    out.flush();
    delay(100);
    rp2040.reboot();
    return true;
  }

  if (strcmp(cmd, "dfu") == 0) {
    out.println(F("Redémarrage en mode bootloader USB (UF2)..."));
    out.flush();
    delay(100);
    rp2040.rebootToBootloader();
    return true;
  }

  if (strcmp(cmd, "defaults") == 0) {
    cmdDefaults(out);
    return true;
  }

  if (strcmp(cmd, "help") == 0) {
    // Pas de texte d'aide sur une liaison radio distante (APRS/mesh) : ça ne
    // ferait que gaspiller l'airtime pour un client qui explore la commande.
    if (isLocal) {
      out.println(F("Commandes: get <key>, set <key> <value>, clockdate JJ/MM/AA HH:MM:SS,"));
      out.println(F("  list, save, status, version, uptime, freemem, beacon, wx,"));
      out.println(F("  send aprs <texte>, history [n], defaults, reboot, dfu, help"));
    }
    return true;
  }

  return false;
}

bool CommandHandler::execute(const char* input, char* outBuf, size_t outLen, bool isLocal) {
  StringPrint sp(outBuf, outLen);
  return execute(input, sp, isLocal);
}

// ============================================================================
// Commandes qui modifient l'état de la station — exigent le mot de passe
// admin en premier mot quand elles arrivent par message APRS.
// ============================================================================
bool CommandHandler::isPrivilegedCommand(const char* cmd)
{
  return strncmp(cmd, "set ", 4) == 0 ||
         strncmp(cmd, "clockdate ", 10) == 0 ||
         strcmp(cmd, "save") == 0 ||
         strcmp(cmd, "reboot") == 0 ||
         strcmp(cmd, "dfu") == 0 ||
         strcmp(cmd, "defaults") == 0 ||
         strcmp(cmd, "history clear") == 0 ||
         strncmp(cmd, "send aprs ", 10) == 0 ||
         strcmp(cmd, "beacon") == 0 ||
         strcmp(cmd, "wx") == 0;
}

// ============================================================================
// Commandes
// ============================================================================
void CommandHandler::cmdGet(const char* key, Print& out) {
  char val[64];
  if (_registry->get(key, val, sizeof(val))) {
    out.printf("%s = %s\n", key, val);
  } else {
    out.printf("Clé inconnue: %s\n", key);
  }
}

void CommandHandler::cmdSet(const char* key, const char* value, Print& out) {
  if (_registry->set(key, value, &out)) {
    char readback[64];
    _registry->get(key, readback, sizeof(readback));
    out.printf("%s = %s (non sauvé, 'save' pour persister)\n", key, readback);

    // Appliquer immédiatement le log_level si modifié
    if (strcmp(key, "system.log_level") == 0) {
      g_log_level = (LogLevel)_settings->system.log_level;
      out.printf("Log level: %s\n", logLevelName(g_log_level));
    }

    // Relais : appliquer immédiatement l'impulsion I2C (la clé venant d'être
    // écrite dans _settings)
    int relayNum = 0;
    char relayField[16] = {0};
    if (sscanf(key, "relay.%d.%15s", &relayNum, relayField) == 2 &&
        strcmp(relayField, "state") == 0 &&
        relayNum >= 1 && relayNum <= RELAY_COUNT) {
      uint8_t idx = relayNum - 1;
      if (!_relay->setState(idx, _settings->relay[idx].state)) {
        out.printf("Attention: expandeur relais (TCA9555) non détecté, pas d'action matérielle\n");
      }
    }
  } else {
    // Si set retourne false mais que la clé existe, c'est une erreur de validation
    // (le message a déjà été affiché par validate())
    if (!_registry->find(key)) {
      out.printf("Clé inconnue: %s\n", key);
    }
  }
}

// ============================================================================
// Règle l'horloge RTC depuis "JJ/MM/AA HH:MM:SS"
// ============================================================================
void CommandHandler::cmdSetClockDate(const char* value, Print& out) {
  int day, month, year, hour, minute, second;
  if (sscanf(value, "%d/%d/%d %d:%d:%d", &day, &month, &year, &hour, &minute, &second) != 6) {
    out.println(F("Usage: clock JJ/MM/AA HH:MM:SS"));
    return;
  }
  if (year < 100) {
    year += 2000;
  }

  DateTime dt(year, month, day, hour, minute, second);
  rtc_clock.setCurrentTime(dt.unixtime());

  out.printf("Horloge réglée: %02d/%02d/%04d %02d:%02d:%02d UTC\n",
    day, month, year, hour, minute, second);
}

void CommandHandler::cmdList(Print& out) {
  out.println(F("--- Configuration ---"));
  _registry->listAll(out);
}

void CommandHandler::cmdSave(Print& out) {
  if (_manager->save(*_settings)) {
    out.println(F("Configuration sauvegardée"));
  } else {
    out.println(F("Erreur sauvegarde"));
  }
}

void CommandHandler::cmdStatus(Print& out) {
  out.println(F("--- Telemetry ---"));
  // out.printf("  Batterie:  %.0f mV / %.0f mA\n", _telemetry->battery.voltage_mv, _telemetry->battery.current_ma);
  // out.printf("  Solaire:   %.0f mV / %.0f mA\n", _telemetry->solar.voltage_mv, _telemetry->solar.current_ma);
  // out.printf("  Board:     %.0f mV / %.0f mA\n", _telemetry->board_5v.voltage_mv, _telemetry->board_5v.current_ma);
  // out.printf("  MPPT:      status=0x%04X\n", _telemetry->mppt_status);
  // out.printf("  Meteo:     %.1fC / %.0f%% / %.1f hPa\n",
  //   _telemetry->weather.temperature_c, _telemetry->weather.humidity, _telemetry->weather.pressure_hpa);
  // out.printf("  WH65B:     vent=%.1f m/s dir=%d rain=%.1fmm UV=%d lux=%.0f\n",
  //   _telemetry->weather.wind_avg_ms, _telemetry->weather.wind_dir_deg,
  //   _telemetry->weather.rain_mm, _telemetry->weather.uv_index, _telemetry->weather.light_lux);
  // TODO revoir cette commande
  out.printf("  Uptime:    %lu s\n", _telemetry->uptime_s);
}

void CommandHandler::cmdVersion(Print& out) {
  out.printf("%s build %s\n", FIRMWARE_VERSION, FIRMWARE_BUILD_DATE);
  out.print(F("Board:"));
  out.println(board.getManufacturerName());
  out.printf("Uptime: %lu s  Free heap: %u\n", _telemetry->uptime_s, (unsigned)rp2040.getFreeHeap());
}

void CommandHandler::cmdDefaults(Print& out) {
  *_settings = getDefaultSettings();
  _registry->init(*_settings);  // re-pointer les entrées
  out.println(F("Valeurs par défaut chargées (non sauvé, 'save' pour persister)"));
}

void CommandHandler::cmdHistory(const char* args, Print& out) {
  if (!_history || !_history->isInitialized()) {
    out.println(F("Historique EEPROM non disponible"));
    return;
  }

  // "history clear" — efface l'historique
  if (strcmp(args, "clear") == 0) {
    _history->clear();
    out.println(F("Historique effacé"));
    return;
  }

  // "history" ou "history N" — affiche les N derniers records (défaut: 20)
  uint16_t n = 20;
  if (args[0] >= '0' && args[0] <= '9') {
    n = strtol(args, nullptr, 10);
  }
  _history->dump(out, n);
}

void CommandHandler::cmdSendAprs(const char* content, Print& out) {
  if (_aprs->sendRaw(content)) {
    out.println(F("Paquet APRS envoyé"));
  } else {
    out.println(F("Échec envoi (contenu vide ou trop long)"));
  }
}

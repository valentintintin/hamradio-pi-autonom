#include "CommandHandler.h"
#include "core/Log.h"
#include "core/Version.h"
#include "core/LockGuard.h"
#include "core/StringPrint.h"
#include "mesh/MeshcoreRepeater.h"
#include <target.h>
#include <RTClib.h>
#include <cstring>
#include <cstdio>

extern LogLevel g_log_level;
extern AprsDispatcher aprs_dispatcher;

// Verrou récursif (partagé avec AprsEngine::getMutex()) : un message APRS reçu
// tient déjà ce verrou quand il appelle execute(), qui peut lui-même rappeler
// AprsEngine ("beacon"/"wx"/"send aprs") — sans récursivité, interblocage.
bool CommandHandler::execute(const char* input, Print& out, bool isLocal) {
  RecursiveLockGuard lock(_mutex);

  char buf[220];
  strncpy(buf, input, sizeof(buf) - 1);
  buf[sizeof(buf) - 1] = '\0';

  char* cmd = buf;
  while (*cmd == ' ') {
    cmd++;
  }

  if (strncmp(cmd, "get ", 4) == 0) {
    cmdGet(cmd + 4, out);
    return true;
  }

  if (strncmp(cmd, "set ", 4) == 0) {
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
      out.println("Usage: set <key> <value>");
    }
    return true;
  }

  // "clockdate" et non "clock"/"time" : évite une collision avec les commandes CLI MeshCore homonymes.
  if (strncmp(cmd, "clockdate ", 10) == 0) {
    cmdSetClockDate(cmd + 10, out);
    return true;
  }

  if (strcmp(cmd, "list") == 0) {
    cmdList(out, isLocal);
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
    out.printf("Heap: %u\n", (unsigned)rp2040.getFreeHeap());
    return true;
  }

  if (strcmp(cmd, "beacon") == 0) {
    out.println(_aprs->sendPosition(_settings->aprs.comment) ? "Position envoyée" : "Échec envoi position");
    return true;
  }

  if (strcmp(cmd, "wx") == 0) {
    out.println(_aprs->sendWeather() ? "Météo envoyée" : "Échec envoi météo");
    return true;
  }

  if (strncmp(cmd, "send aprs ", 10) == 0) {
    cmdSendAprs(cmd + 10, out);
    return true;
  }

  // L'upload binaire "image <n>" est intercepté en amont par tasks/task_cli.cpp et n'atteint jamais ce dispatch.
  if (strncmp(cmd, "image", 5) == 0 && (cmd[5] == '\0' || cmd[5] == ' ')) {
    const char* args = (cmd[5] == ' ') ? cmd + 6 : "";
    cmdImage(args, out, isLocal);
    return true;
  }

  if (strncmp(cmd, "history", 7) == 0) {
    const char* args = (cmd[7] == ' ') ? cmd + 8 : "";
    cmdHistory(args, out, isLocal);
    return true;
  }

  if (strncmp(cmd, "eventlog", 8) == 0) {
    const char* args = (cmd[8] == ' ') ? cmd + 9 : "";
    cmdEventLog(args, out, isLocal);
    return true;
  }

  if (strncmp(cmd, "relay ", 6) == 0) {
    cmdRelay(cmd + 6, out);
    return true;
  }

  if (strcmp(cmd, "reboot") == 0) {
    out.println("Redémarrage...");
    out.flush();
    delay(100);
    rp2040.reboot();
    return true;
  }

  if (strcmp(cmd, "dfu") == 0) {
    out.println("Redémarrage bootloader USB...");
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
    // Pas de texte d'aide à distance : gaspillerait l'airtime radio.
    if (isLocal) {
      out.println("Commandes: get <key>, set <key> <value>, clockdate JJ/MM/AA HH:MM:SS,");
      out.println("  list, save, status, version, uptime, freemem, beacon, wx,");
      out.println("  send aprs <texte>, history [n]/clear/dump [n], eventlog [n]/clear/dump [n],");
      out.println("  image <n> (upload binaire série)/send/cancel, relay <n> on/off/auto, defaults, reboot, dfu, help");
    }
    return true;
  }

  return false;
}

bool CommandHandler::execute(const char* input, char* outBuf, size_t outLen, bool isLocal) {
  StringPrint sp(outBuf, outLen);
  return execute(input, sp, isLocal);
}

bool CommandHandler::isPrivilegedCommand(const char* cmd)
{
  return strncmp(cmd, "set ", 4) == 0 ||
         strncmp(cmd, "clockdate ", 10) == 0 ||
         strcmp(cmd, "save") == 0 ||
         strcmp(cmd, "reboot") == 0 ||
         strcmp(cmd, "dfu") == 0 ||
         strcmp(cmd, "defaults") == 0 ||
         strcmp(cmd, "history clear") == 0 ||
         strcmp(cmd, "eventlog clear") == 0 ||
         strncmp(cmd, "send aprs ", 10) == 0 ||
         strcmp(cmd, "beacon") == 0 ||
         strcmp(cmd, "wx") == 0 ||
         strcmp(cmd, "image send") == 0 ||
         strncmp(cmd, "relay ", 6) == 0;
}

void CommandHandler::cmdGet(const char* key, Print& out) {
  char val[64];
  if (_registry->get(key, val, sizeof(val))) {
    out.printf("%s = %s\n", key, val);
  } else {
    out.printf("Inconnue: %s\n", key);
  }
}

void CommandHandler::cmdSet(const char* key, const char* value, Print& out) {
  if (_registry->set(key, value, &out)) {
    char readback[64];
    _registry->get(key, readback, sizeof(readback));
    out.printf("%s=%s (non sauvé)\n", key, readback);

    if (strcmp(key, "system.log_level") == 0) {
      g_log_level = (LogLevel)_settings->system.log_level;
      out.printf("Log: %s\n", logLevelName(g_log_level));
    }

    if (strcmp(key, "system.mode") == 0) {
      out.printf("Mode: %s (reboot requis)\n", modeName(_settings->system.mode));
    }

    if (strcmp(key, "system.telemetry_log.enabled") == 0 && _history) {
      _history->setEnabled(_settings->system.telemetry_log_enabled);
    }
    if (strcmp(key, "system.event_log.enabled") == 0 && _event_log) {
      _event_log->setEnabled(_settings->system.event_log_enabled);
    }

    if (_mppt && strcmp(key, "energy.mppt_pwr_off_mv") == 0) {
      if (!_mppt->setPowerOffThreshold(_settings->energy.mppt_pwr_off_mv)) {
        out.printf("MPPT: non détecté/échec\n");
      }
    }
    if (_mppt && strcmp(key, "energy.mppt_pwr_on_mv") == 0) {
      if (!_mppt->setPowerOnThreshold(_settings->energy.mppt_pwr_on_mv)) {
        out.printf("MPPT: non détecté/échec\n");
      }
    }

    // pause()/resume() : un re-begin() du modem à froid pourrait corrompre un paquet en cours d'émission.
    if ((strcmp(key, "radio.aprs.freq") == 0 || strcmp(key, "radio.aprs.bw") == 0 ||
         strcmp(key, "radio.aprs.sf") == 0 || strcmp(key, "radio.aprs.cr") == 0 ||
         strcmp(key, "radio.aprs.power") == 0 || strcmp(key, "radio.aprs.cad") == 0) &&
        modeHasAprs(_settings->system.mode)) {
      aprs_dispatcher.pause();
      bool ok = aprs_radio_hw.switchToLora();
      aprs_dispatcher.resume();
      if (!ok) {
        out.printf("Radio: échec reconfig\n");
      }
    }

    if (_mesh && strncmp(key, "mesh.channel.", 13) == 0 && modeHasMeshcore(_settings->system.mode)) {
      _mesh->loadChannelsFromSettings(*_settings);
    }
  } else {
    if (!_registry->find(key)) {
      out.printf("Inconnue: %s\n", key);
    }
  }
}

// Seul point d'entrée pour piloter un relais : l'état ON/OFF n'est pas une setting persistée (cf. Settings::Relay).
void CommandHandler::cmdRelay(const char* args, Print& out) {
  int relayNum = 0;
  char verb[16] = {0};
  if (sscanf(args, "%d %15s", &relayNum, verb) != 2 ||
      relayNum < 1 || relayNum > RELAY_COUNT) {
    out.println("Usage: relay <n> on|off|auto");
    return;
  }

  uint8_t idx = relayNum - 1;

  if (strcmp(verb, "on") == 0 || strcmp(verb, "off") == 0) {
    bool on = strcmp(verb, "on") == 0;
    if (!_relays[idx].setManualState(on, _settings->energy.relay_manual_timeout_ms)) {
      out.printf("Relais: TCA9555 non détecté\n");
    }
    out.printf("Relais %d=%s (manuel)\n", relayNum, on ? "ON" : "OFF");
    if (_event_log) {
      _event_log->log(EVENT_RELAY_MANUAL_SET, relayNum, on ? 1 : 0);
    }
    return;
  }

  if (strcmp(verb, "auto") != 0) {
    out.println("Usage: relay <n> on|off|auto");
    return;
  }

  _relays[idx].setManualOverride(false);
  out.printf("Relais %d=auto\n", relayNum);
  if (_event_log) {
    _event_log->log(EVENT_RELAY_MANUAL_CLEARED, relayNum, 0);
  }
}

void CommandHandler::cmdSetClockDate(const char* value, Print& out) {
  int day, month, year, hour, minute, second;
  if (sscanf(value, "%d/%d/%d %d:%d:%d", &day, &month, &year, &hour, &minute, &second) != 6) {
    out.println("Usage: clockdate JJ/MM/AA HH:MM:SS");
    return;
  }
  if (year < 100) {
    year += 2000;
  }

  DateTime dt(year, month, day, hour, minute, second);
  rtc_clock.setCurrentTime(dt.unixtime());

  // Sans ça, l'heure serait reperdue au boot après coupure d'alimentation (RTC interne RP2040 non battery-backed).
  externalRtc.writeTime(dt.unixtime());

  out.printf("RTC: %02d/%02d/%04d %02d:%02d:%02d UTC\n",
    day, month, year, hour, minute, second);
}

void CommandHandler::cmdList(Print& out, bool isLocal) {
  if (isLocal) {
    out.println("--- Configuration ---");
  }
  _registry->listAll(out);
}

void CommandHandler::cmdSave(Print& out) {
  if (_manager->save(*_settings)) {
    out.println("Sauvegardé");
  } else {
    out.println("Erreur save");
  }
}

void CommandHandler::cmdStatus(Print& out) {
  out.printf("Uptime: %lu s\n", _telemetry->uptime_s);
}

void CommandHandler::cmdVersion(Print& out) {
  out.printf("%s %s (%s)\n", FIRMWARE_VERSION, FIRMWARE_BUILD_DATE, board.getManufacturerName());
  out.printf("Up %lus Heap %u\n", _telemetry->uptime_s, (unsigned)rp2040.getFreeHeap());
}

void CommandHandler::cmdDefaults(Print& out) {
  *_settings = getDefaultSettings();
  _registry->init(*_settings);  // le registre pointe dans l'ancienne struct, il faut re-pointer
  out.println("Défauts chargés (non sauvé)");
}

void CommandHandler::cmdHistory(const char* args, Print& out, bool isLocal) {
  if (!_history || !_history->isInitialized()) {
    out.println("Historique indisponible");
    return;
  }

  if (strcmp(args, "clear") == 0) {
    _history->clear();
    out.println("Historique effacé");
    return;
  }

  // Dump binaire brut : casserait le tampon texte APRS/mesh, réservé au port série local.
  if (strcmp(args, "dump") == 0 || strncmp(args, "dump ", 5) == 0) {
    if (!isLocal) {
      out.println("Série uniquement");
      return;
    }
    uint16_t n = 0;
    const char* nArg = (args[4] == ' ') ? args + 5 : "";
    if (nArg[0] >= '0' && nArg[0] <= '9') {
      n = strtol(nArg, nullptr, 10);
    }
    _history->dumpBinary(out, n);
    return;
  }

  uint16_t n = 1;
  if (args[0] >= '0' && args[0] <= '9') {
    n = strtol(args, nullptr, 10);
  }

  // Tampon de réponse distant minuscule (~100-160 octets) : seul le dernier tient.
  if (!isLocal) {
    if (n != 1) {
      out.println("Distant: dernier seul");
      return;
    }
    _history->dump(out, 1, false);
    return;
  }

  _history->dump(out, n);
}

void CommandHandler::cmdEventLog(const char* args, Print& out, bool isLocal) {
  if (!_event_log || !_event_log->isInitialized()) {
    out.println("Eventlog indisponible");
    return;
  }

  if (strcmp(args, "clear") == 0) {
    _event_log->clear();
    out.println("Eventlog effacé");
    return;
  }

  if (strcmp(args, "dump") == 0 || strncmp(args, "dump ", 5) == 0) {
    if (!isLocal) {
      out.println("Série uniquement");
      return;
    }
    uint16_t n = 0;
    const char* nArg = (args[4] == ' ') ? args + 5 : "";
    if (nArg[0] >= '0' && nArg[0] <= '9') {
      n = strtol(nArg, nullptr, 10);
    }
    _event_log->dumpBinary(out, n);
    return;
  }

  uint16_t n = 1;
  if (args[0] >= '0' && args[0] <= '9') {
    n = strtol(args, nullptr, 10);
  }

  if (!isLocal && n != 1) {
    out.println("Distant: dernier seul");
    return;
  }

  uint16_t total = _event_log->getCount();
  uint16_t count = (n > 0 && n < total) ? n : total;
  uint16_t start = total - count;

  if (isLocal) {
    out.printf("Eventlog: %d/%d\n", total, _event_log->getMaxRecords());
  }
  EventLogRecord rec{};
  for (uint16_t i = start; i < total; i++) {
    if (_event_log->readRecord(i, rec)) {
      out.printf("%lu %s data=%ld,%ld,%ld,%ld,%ld\n",
        (unsigned long)rec.timestamp, eventCodeName(rec.code),
        (long)rec.data0, (long)rec.data1, (long)rec.data2, (long)rec.data3, (long)rec.data4);
    }
  }
}

void CommandHandler::cmdSendAprs(const char* content, Print& out) {
  if (_aprs->sendRaw(content)) {
    out.println("Envoyé");
  } else {
    out.println("Échec envoi");
  }
}

void CommandHandler::cmdImage(const char* args, Print& out, bool isLocal) {
  if (!modeHasAprs(_settings->system.mode)) {
    out.println("APRS inactif");
    return;
  }
  if (!_sstv) {
    out.println("SSTV non disponible");
    return;
  }

  // N'arme qu'une demande, l'émission tourne dans tasks/task_sstv.cpp.
  if (strcmp(args, "send") == 0) {
    _sstv->requestTransmit(&out);
    return;
  }

  if (strcmp(args, "cancel") == 0) {
    _sstv->cancelUpload();
    out.println("Upload annulé");
    return;
  }

  out.println("Usage: image <n>/send/cancel");
}

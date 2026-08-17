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
      out.println("Usage: set <key> <value>");
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

  // "image send"/"image cancel" uniquement ici : l'upload binaire ("image
  // <n>") est intercepté en amont par tasks/task_cli.cpp (série uniquement,
  // bascule la lecture en mode binaire) et n'atteint jamais ce dispatch.
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
    out.println("Redémarrage en mode bootloader USB (UF2)...");
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
         strcmp(cmd, "eventlog clear") == 0 ||
         strncmp(cmd, "send aprs ", 10) == 0 ||
         strcmp(cmd, "beacon") == 0 ||
         strcmp(cmd, "wx") == 0 ||
         strcmp(cmd, "image send") == 0 ||
         strncmp(cmd, "relay ", 6) == 0;
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

    // Mode de fonctionnement : ne prend effet qu'au reboot (les tâches
    // FreeRTOS ne sont créées qu'une fois, dans setup() — cf. core/Boot.cpp)
    if (strcmp(key, "system.mode") == 0) {
      out.printf("Mode: %s (effectif après 'save' + 'reboot')\n", modeName(_settings->system.mode));
    }

    // Activation/désactivation de la télémétrie/journalisation EEPROM :
    // appliquer immédiatement (pas besoin de reboot, juste un court-circuit
    // de record()/log(), cf. hal/eeprom/TelemetryHistory.h/EventLogHistory.h)
    if (strcmp(key, "system.telemetry_log.enabled") == 0 && _history) {
      _history->setEnabled(_settings->system.telemetry_log_enabled);
    }
    if (strcmp(key, "system.event_log.enabled") == 0 && _event_log) {
      _event_log->setEnabled(_settings->system.event_log_enabled);
    }

    // MPPT : pousser immédiatement les seuils de coupure/reprise matériels
    // (registres du chip, pas juste des valeurs relues périodiquement)
    if (_mppt && strcmp(key, "energy.mppt_pwr_off_mv") == 0) {
      if (!_mppt->setPowerOffThreshold(_settings->energy.mppt_pwr_off_mv)) {
        out.printf("Attention: MPPT non détecté ou écriture échouée, pas d'action matérielle\n");
      }
    }
    if (_mppt && strcmp(key, "energy.mppt_pwr_on_mv") == 0) {
      if (!_mppt->setPowerOnThreshold(_settings->energy.mppt_pwr_on_mv)) {
        out.printf("Attention: MPPT non détecté ou écriture échouée, pas d'action matérielle\n");
      }
    }

    // Fréquence/bande/SF/CR/Puissance/CAD : appliquer immédiatement plutôt que
    // d'attendre le prochain retour LoRa (cycle météo, transmission CW/SSTV) ou
    // un reboot. Un re-begin() complet du modem pouvant corrompre un paquet en
    // cours, on encadre par pause()/resume() (même précaution que
    // task_weather.cpp/SstvTransmitter.cpp).
    if ((strcmp(key, "radio.aprs.freq") == 0 || strcmp(key, "radio.aprs.bw") == 0 ||
         strcmp(key, "radio.aprs.sf") == 0 || strcmp(key, "radio.aprs.cr") == 0 ||
         strcmp(key, "radio.aprs.power") == 0 || strcmp(key, "radio.aprs.cad") == 0) &&
        modeHasAprs(_settings->system.mode)) {
      aprs_dispatcher.pause();
      bool ok = aprs_radio_hw.switchToLora();
      aprs_dispatcher.resume();
      if (!ok) {
        out.printf("Attention: échec reconfiguration radio\n");
      }
    }

    // Canaux de groupe MeshCore : recharger immédiatement plutôt que d'attendre
    // un reboot — juste dérive le secret/hash depuis le nom (sha256) et écrit
    // dans le tableau en RAM de MeshcoreRepeater, aucun accès radio/mutex
    // supplémentaire donc rien à encadrer ici.
    if (_mesh && strncmp(key, "mesh.channel.", 13) == 0 && modeHasMeshcore(_settings->system.mode)) {
      _mesh->loadChannelsFromSettings(*_settings);
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
// "relay <n> on|off|auto" — seul point d'entrée pour piloter un relais.
// Pas de clé "set relay.N.state" (l'état ON/OFF n'est pas une settings
// persistée, cf. Settings::Relay). "on"/"off" bascule le matériel
// (RelayHal::setManualState, qui met à jour l'état connu — RAM + mirroir
// scratch, cf. RelayHal::begin()) et arme l'override manuel :
// Relay::updateCutoff/updatePeriodic (hal/relay/Relay.cpp) cessent de
// reprendre la main sur ce relais jusqu'à "relay <n> auto".
// ============================================================================
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
    if (!_relay->setManualState(idx, on)) {
      out.printf("Attention: expandeur relais (TCA9555) non détecté, pas d'action matérielle\n");
    }
    out.printf("Relais %d = %s (mode manuel, 'relay %d auto' pour rendre la main)\n",
      relayNum, on ? "ON" : "OFF", relayNum);
    if (_event_log) {
      _event_log->log(EVENT_RELAY_MANUAL_SET, relayNum, on ? 1 : 0);
    }
    return;
  }

  if (strcmp(verb, "auto") != 0) {
    out.println("Usage: relay <n> on|off|auto");
    return;
  }

  _relay->clearManualOverride(idx);
  out.printf("Relais %d rendu à l'automatisme (cutoff/périodique actifs)\n", relayNum);
  if (_event_log) {
    _event_log->log(EVENT_RELAY_MANUAL_CLEARED, relayNum, 0);
  }
}

// ============================================================================
// Règle l'horloge RTC depuis "JJ/MM/AA HH:MM:SS"
// ============================================================================
void CommandHandler::cmdSetClockDate(const char* value, Print& out) {
  int day, month, year, hour, minute, second;
  if (sscanf(value, "%d/%d/%d %d:%d:%d", &day, &month, &year, &hour, &minute, &second) != 6) {
    out.println("Usage: clock JJ/MM/AA HH:MM:SS");
    return;
  }
  if (year < 100) {
    year += 2000;
  }

  DateTime dt(year, month, day, hour, minute, second);
  rtc_clock.setCurrentTime(dt.unixtime());

  // Persiste aussi vers la puce RTC externe battery-backed si présente (cf.
  // hal/rtc/ExternalRtc.h) : sans ça, l'heure réglée ici serait reperdue au
  // prochain boot après une coupure d'alimentation (fallback RP2040 interne
  // non battery-backed).
  externalRtc.writeTime(dt.unixtime());

  out.printf("Horloge réglée: %02d/%02d/%04d %02d:%02d:%02d UTC\n",
    day, month, year, hour, minute, second);
}

void CommandHandler::cmdList(Print& out) {
  out.println("--- Configuration ---");
  _registry->listAll(out);
}

void CommandHandler::cmdSave(Print& out) {
  if (_manager->save(*_settings)) {
    out.println("Configuration sauvegardée");
  } else {
    out.println("Erreur sauvegarde");
  }
}

void CommandHandler::cmdStatus(Print& out) {
  out.println("--- Telemetry ---");
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
  out.print("Board:");
  out.println(board.getManufacturerName());
  out.printf("Uptime: %lu s  Free heap: %u\n", _telemetry->uptime_s, (unsigned)rp2040.getFreeHeap());
}

void CommandHandler::cmdDefaults(Print& out) {
  *_settings = getDefaultSettings();
  _registry->init(*_settings);  // re-pointer les entrées
  out.println("Valeurs par défaut chargées (non sauvé, 'save' pour persister)");
}

void CommandHandler::cmdHistory(const char* args, Print& out, bool isLocal) {
  if (!_history || !_history->isInitialized()) {
    out.println("Historique EEPROM non disponible");
    return;
  }

  // "history clear" — efface l'historique
  if (strcmp(args, "clear") == 0) {
    _history->clear();
    out.println("Historique effacé");
    return;
  }

  // "history dump [n]" — dump binaire brut (EepromDumpHeader + records),
  // pour récupération efficace par un outil côté PC. Réservé au port série
  // local : sortie brute non formatée, ça n'a pas de sens sur une liaison
  // radio (et casserait le tampon texte APRS/mesh).
  if (strcmp(args, "dump") == 0 || strncmp(args, "dump ", 5) == 0) {
    if (!isLocal) {
      out.println("history dump: série uniquement");
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

  // "history" ou "history N" — affiche les N derniers records (défaut: 1,
  // le plus récent — sans argument, c'est toujours "le dernier")
  uint16_t n = 1;
  if (args[0] >= '0' && args[0] <= '9') {
    n = strtol(args, nullptr, 10);
  }

  // En distant (APRS/mesh), le tampon de réponse est minuscule (~100-160
  // octets) : seul le dernier record tient, et sans bannière/en-tête CSV.
  if (!isLocal) {
    if (n != 1) {
      out.println("history: seul le dernier ('history' ou 'history 1') est autorisé en distant");
      return;
    }
    _history->dump(out, 1, false);
    return;
  }

  _history->dump(out, n);
}

void CommandHandler::cmdEventLog(const char* args, Print& out, bool isLocal) {
  if (!_event_log || !_event_log->isInitialized()) {
    out.println("Log événements EEPROM non disponible");
    return;
  }

  if (strcmp(args, "clear") == 0) {
    _event_log->clear();
    out.println("Log événements effacé");
    return;
  }

  // "eventlog dump [n]" — même principe que "history dump", cf. commentaire
  // ci-dessus.
  if (strcmp(args, "dump") == 0 || strncmp(args, "dump ", 5) == 0) {
    if (!isLocal) {
      out.println("eventlog dump: série uniquement");
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

  // "eventlog" ou "eventlog N" — affiche les N derniers événements (défaut:
  // 1, le plus récent — sans argument, c'est toujours "le dernier")
  uint16_t n = 1;
  if (args[0] >= '0' && args[0] <= '9') {
    n = strtol(args, nullptr, 10);
  }

  // En distant (APRS/mesh), le tampon de réponse est minuscule (~100-160
  // octets) : seul le dernier événement tient, et sans bannière.
  if (!isLocal && n != 1) {
    out.println("eventlog: seul le dernier ('eventlog' ou 'eventlog 1') est autorisé en distant");
    return;
  }

  uint16_t total = _event_log->getCount();
  uint16_t count = (n > 0 && n < total) ? n : total;
  uint16_t start = total - count;

  if (isLocal) {
    out.printf("--- Log événements: %d/%d records ---\n", total, _event_log->getMaxRecords());
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
    out.println("Paquet APRS envoyé");
  } else {
    out.println("Échec envoi (contenu vide ou trop long)");
  }
}

void CommandHandler::cmdImage(const char* args, Print& out, bool isLocal) {
  if (!modeHasAprs(_settings->system.mode)) {
    out.println("APRS non actif dans ce mode");
    return;
  }
  if (!_sstv) {
    out.println("SSTV non disponible");
    return;
  }

  // "image send" : local ET distant (APRS/mesh) — n'arme qu'une demande,
  // l'émission tourne dans tasks/task_sstv.cpp (cf. aprs/SstvTransmitter.h).
  if (strcmp(args, "send") == 0) {
    _sstv->requestTransmit(&out);
    return;
  }

  // "image cancel" : abandonne un upload en cours
  if (strcmp(args, "cancel") == 0) {
    _sstv->cancelUpload();
    out.println("Upload annulé");
    return;
  }

  out.println("Usage: image <n> (upload, série uniquement) | image send | image cancel");
}

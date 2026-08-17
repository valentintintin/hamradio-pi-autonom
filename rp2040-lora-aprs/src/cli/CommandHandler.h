#pragma once

#include "config/SettingsRegistry.h"
#include "config/SettingsManager.h"
#include "hal/Telemetry.h"
#include "hal/eeprom/TelemetryHistory.h"
#include "hal/eeprom/EventLogHistory.h"
#include "hal/relay/Relay.h"
#include "hal/chargers/MpptChargerHal.h"
#include "aprs/AprsEngine.h"
#include "aprs/SstvTransmitter.h"
#include <Arduino.h>
#include <FreeRTOS.h>
#include <semphr.h>

class MeshcoreRepeater;

constexpr size_t CLI_RADIO_REPLY_MAX_LEN = 100;

class CommandHandler {
public:
  CommandHandler(Settings& settings, SettingsRegistry& registry,
                 SettingsManager& manager, TelemetryData& telemetry,
                 AprsEngine& aprsEngine, Relay* relays,
                 TelemetryHistory* history = nullptr,
                 MpptChargerHal* mppt = nullptr,
                 EventLogHistory* eventLog = nullptr,
                 SstvTransmitter* sstv = nullptr,
                 MeshcoreRepeater* mesh = nullptr)
    : _settings(&settings), _registry(&registry),
      _manager(&manager), _telemetry(&telemetry),
      _aprs(&aprsEngine), _relays(relays), _history(history), _mppt(mppt),
      _event_log(eventLog), _sstv(sstv), _mesh(mesh), _mutex(aprsEngine.getMutex()) {}

  bool execute(const char* input, Print& out, bool isLocal = true);

  bool execute(const char* input, char* outBuf, size_t outLen, bool isLocal = false);

  bool isPrivilegedCommand(const char* cmd);

private:
  Settings* _settings;
  SettingsRegistry* _registry;
  SettingsManager* _manager;
  TelemetryData* _telemetry;
  AprsEngine* _aprs;
  Relay* _relays;
  TelemetryHistory* _history;
  MpptChargerHal* _mppt;
  EventLogHistory* _event_log;
  SstvTransmitter* _sstv;
  MeshcoreRepeater* _mesh;

  // Partagé avec AprsEngine::getMutex() (pas un mutex séparé) pour éviter un interblocage AB-BA.
  SemaphoreHandle_t _mutex;

  void cmdGet(const char* key, Print& out);
  void cmdSet(const char* key, const char* value, Print& out);
  void cmdSetClockDate(const char* value, Print& out);
  void cmdList(Print& out, bool isLocal);
  void cmdSave(Print& out);
  void cmdStatus(Print& out);
  void cmdVersion(Print& out);
  void cmdDefaults(Print& out);
  void cmdHistory(const char* args, Print& out, bool isLocal);
  void cmdEventLog(const char* args, Print& out, bool isLocal);
  void cmdSendAprs(const char* content, Print& out);
  void cmdImage(const char* args, Print& out, bool isLocal);
  void cmdRelay(const char* args, Print& out);
};

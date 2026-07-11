#pragma once

#include "SettingsRegistry.h"
#include "SettingsManager.h"
#include "hal/Telemetry.h"
#include "hal/TelemetryHistory.h"
#include <Arduino.h>

// ============================================================================
// CommandHandler — parse et exécute les commandes get/set/list/save/reboot
//
// Peut être appelé depuis :
//   - Serial (task_cli)
//   - Message APRS (AprsEngine callback)
//   - Message MeshCore (MyMesh callback)
//
// Commandes supportées :
//   get <key>           → affiche la valeur
//   set <key> <value>   → modifie la valeur (en RAM)
//   save                → persiste sur LittleFS + EEPROM
//   list                → affiche toutes les clés
//   status              → affiche la telemetry courante
//   reboot              → redémarre le RP2040
//   defaults            → recharge les valeurs par défaut
// ============================================================================

class CommandHandler {
public:
  CommandHandler(Settings& settings, SettingsRegistry& registry,
                 SettingsManager& manager, TelemetryData& telemetry,
                 TelemetryHistory* history = nullptr)
    : _settings(&settings), _registry(&registry),
      _manager(&manager), _telemetry(&telemetry), _history(history) {}

  // Exécute une commande, écrit la réponse dans out
  // Retourne true si la commande a été reconnue
  bool execute(const char* input, Print& out);

  // Version qui écrit dans un buffer (pour réponse APRS/mesh)
  bool execute(const char* input, char* outBuf, size_t outLen);

private:
  Settings* _settings;
  SettingsRegistry* _registry;
  SettingsManager* _manager;
  TelemetryData* _telemetry;
  TelemetryHistory* _history;

  void cmdGet(const char* key, Print& out);
  void cmdSet(const char* key, const char* value, Print& out);
  void cmdList(Print& out);
  void cmdSave(Print& out);
  void cmdStatus(Print& out);
  void cmdDefaults(Print& out);
  void cmdHistory(const char* args, Print& out);
};

#pragma once

#include "SettingsRegistry.h"
#include "SettingsManager.h"
#include "hal/Telemetry.h"
#include "hal/TelemetryHistory.h"
#include "hal/RelayHal.h"
#include "aprs/AprsEngine.h"
#include <Arduino.h>
#include <FreeRTOS.h>
#include <semphr.h>

// ============================================================================
// CommandHandler — parse et exécute les commandes get/set/list/save/reboot
//
// Peut être appelé depuis :
//   - Serial (task_cli)
//   - Message APRS (AprsEngine callback)
//   - Message MeshCore (MyMesh callback, en repli si sa propre CLI ne
//     reconnaît pas la commande — cf task_cli)
//
// Commandes supportées :
//   get <key>           → affiche la valeur
//   set <key> <value>   → modifie la valeur (en RAM)
//   clockdate JJ/MM/AA HH:MM:SS → règle l'horloge RTC
//   save                → persiste sur LittleFS + EEPROM
//   list                → affiche toutes les clés
//   status              → affiche la telemetry courante
//   version             → version firmware / uptime / mémoire libre
//   uptime / freemem     → diagnostics rapides
//   beacon / wx          → force l'envoi immédiat position / météo APRS
//   send aprs <texte>    → envoi manuel d'un paquet APRS brut
//   history [n]         → affiche l'historique télémétrie EEPROM
//   reboot              → redémarre le RP2040
//   dfu                 → redémarre en mode bootloader USB (reflash UF2)
//   defaults            → recharge les valeurs par défaut
// ============================================================================

class CommandHandler {
public:
  CommandHandler(Settings& settings, SettingsRegistry& registry,
                 SettingsManager& manager, TelemetryData& telemetry,
                 AprsEngine& aprsEngine, RelayHal& relay,
                 TelemetryHistory* history = nullptr)
    : _settings(&settings), _registry(&registry),
      _manager(&manager), _telemetry(&telemetry),
      _aprs(&aprsEngine), _relay(&relay), _history(history),
      _mutex(aprsEngine.getMutex()) {}

  // Exécute une commande, écrit la réponse dans out.
  // Retourne true si la commande a été reconnue.
  // isLocal : true pour la liaison série (locale, de confiance) ; certaines
  // commandes (ex: "help") ne produisent une réponse que si isLocal, pour ne
  // pas gaspiller l'airtime sur une liaison radio distante (APRS/mesh).
  bool execute(const char* input, Print& out, bool isLocal = true);

  // Version qui écrit dans un buffer (pour réponse APRS/mesh) ; isLocal=false
  // par défaut car ce chemin est toujours utilisé pour une liaison radio.
  bool execute(const char* input, char* outBuf, size_t outLen, bool isLocal = false);

private:
  Settings* _settings;
  SettingsRegistry* _registry;
  SettingsManager* _manager;
  TelemetryData* _telemetry;
  AprsEngine* _aprs;
  RelayHal* _relay;
  TelemetryHistory* _history;

  // Settings/SettingsRegistry/relais sont partagés entre les tâches série,
  // APRS et mesh (MeshcoreRepeater retombe désormais aussi sur execute()) :
  // sans ce verrou, deux commandes concurrentes (ex: "save" en série pendant
  // qu'un "set" arrive par mesh) pourraient entrelacer leurs écritures dans
  // Settings ou pendant la sérialisation LittleFS/EEPROM.
  // Partagé (pas un mutex séparé) avec AprsEngine::getMutex() — voir le
  // commentaire de cette méthode pour la raison (éviter un interblocage AB-BA).
  SemaphoreHandle_t _mutex;

  void cmdGet(const char* key, Print& out);
  void cmdSet(const char* key, const char* value, Print& out);
  void cmdSetClockDate(const char* value, Print& out);
  void cmdList(Print& out);
  void cmdSave(Print& out);
  void cmdStatus(Print& out);
  void cmdVersion(Print& out);
  void cmdDefaults(Print& out);
  void cmdHistory(const char* args, Print& out);
  void cmdSendAprs(const char* content, Print& out);
};

#include "Boot.h"

#include <Arduino.h>
#include <LittleFS.h>

#include "target.h"
#include "tasks/tasks.h"

// MeshCore core
#include <helpers/ArduinoHelpers.h>
#include <helpers/IdentityStore.h>

#include "mesh/MeshcoreRepeater.h"
#include "aprs/AprsDispatcher.h"
#include "aprs/AprsEngine.h"
#include "aprs/AprsEventHandler.h"
#include "aprs/LoRa433RadioMode.h"
#include "hal/i2c/I2CBus.h"
#include "hal/sensors/Ina3221Hal.h"
#include "hal/chargers/MpptChargerHal.h"
#include "hal/sensors/Bme280Hal.h"
#include "hal/chargers/VictronHal.h"
#include "hal/chargers/ChargeControllerHal.h"
#include "hal/eeprom/M24M01Hal.h"
#include "hal/eeprom/TelemetryHistory.h"
#include "hal/eeprom/EventLogHistory.h"
#include "hal/relay/RelayHal.h"
#include "core/Log.h"
#include "config/Settings.h"
#include "config/SettingsManager.h"
#include "config/SettingsRegistry.h"
#include "aprs/SstvTransmitter.h"

// ============================================================================
// Objets globaux déclarés dans main.cpp (racine de composition) — mêmes
// externs que ceux utilisés par tasks/task_*.cpp.
// ============================================================================
extern LogLevel g_log_level;
extern StdRNG fast_rng;
extern Settings settings;
extern MeshcoreRepeater the_mesh;
extern AprsDispatcher aprs_dispatcher;
extern AprsEngine aprs_engine;
extern AprsEventHandler aprs_event_handler;
extern I2CBus i2c_bus;
extern Ina3221Hal ina3221;
extern MpptChargerHal mppt;
extern Bme280Hal bme280;
extern VictronHal victron;
extern M24M01Hal eeprom;
extern ChargeControllerHal* active_charger;
extern TelemetryHistory telemetry_history;
extern EventLogHistory event_log;
extern RelayHal relay_hal;
extern SettingsManager settings_manager;
extern SettingsRegistry settings_registry;
extern SstvTransmitter sstv_transmitter;

// ============================================================================
// Étapes de boot
// ============================================================================

void bootInitCore() {
  Serial.begin(115200);
  delay(2000); // attendre USB serial

  board.begin();
  LittleFS.begin();
}

// I2C bus + EEPROM seuls : nécessaire dans TOUS les modes pour que
// bootLoadConfig() puisse retomber sur l'EEPROM si LittleFS est vide/corrompu
// — à cet instant settings.system.mode n'est pas encore connu.
void bootInitEeprom() {
  i2c_bus.begin();
  LOG_I("I2C", "Bus initialisé");

  if (eeprom.begin()) {
    LOG_I("I2C", "EEPROM M24M01 OK");
  } else {
    LOG_W("I2C", "EEPROM non détectée");
  }
}

void bootLoadConfig() {
  settings = settings_manager.load();
  settings_registry.init(settings);
  sstv_transmitter.init(settings);
  g_log_level = (LogLevel)settings.system.log_level;
  LOG_I("CONFIG", "%d paramètres, log=%s, mode=%s",
    settings_registry.count(), logLevelName(g_log_level), modeName(settings.system.mode));
}

void bootInitRadios() {
  uint8_t mode = settings.system.mode;

  if (modeHasMeshcore(mode)) {
    if (!mesh_radio_init()) {
      LOG_E("RADIO", "Init radio 868 FAIL");
    } else {
      LOG_I("RADIO", "Init radio 868 OK");
    }
  }

  if (modeHasAprs(mode)) {
    if (!aprs_radio_init()) {
      LOG_E("RADIO", "Init radio 433 FAIL");
    } else {
      LOG_I("RADIO", "Init radio 433 OK");
    }
  }
}

// RNG et identité MeshCore utilisent le bruit de la radio 868 (cf.
// variant/target.cpp: radio_get_rng_seed/radio_new_identity) — inutiles (et
// la radio non initialisée) si MeshCore n'est pas actif dans ce mode.
void bootSeedRng() {
  if (!modeHasMeshcore(settings.system.mode)) {
    return;
  }
  fast_rng.begin(radio_get_rng_seed());
}

void bootInitIdentity() {
  if (!modeHasMeshcore(settings.system.mode)) {
    return;
  }

  IdentityStore store(LittleFS, "/identity");
  store.begin();

  if (!store.load("_main", the_mesh.self_id)) {
    LOG_W("MESH", "Génération nouvelle identité");
    the_mesh.self_id = radio_new_identity(); // create new random identity
    int count = 0;
    while (count < 10 && (the_mesh.self_id.pub_key[0] == 0x00 || the_mesh.self_id.pub_key[0] == 0xFF)) {  // reserved id hashes
      the_mesh.self_id = radio_new_identity();
      count++;
    }
    store.save("_main", the_mesh.self_id);
  }

  LOG_I("MESH", "Repeater ID: ");
  mesh::Utils::printHex(Serial, the_mesh.self_id.pub_key, PUB_KEY_SIZE);
}

// Capteurs/chargeurs/historique EEPROM : partie "télémétrie" du mode complet
// uniquement (relais + télémétrie = fonctionnement normal, cf. Settings.h).
// L'I2C bus et l'EEPROM elle-même sont déjà initialisés par bootInitEeprom()
// (nécessaire dans tous les modes pour le fallback settings).
void bootInitSensors() {
  if (!modeIsFull(settings.system.mode)) {
    return;
  }

  if (ina3221.begin()) {
    LOG_I("I2C", "INA3221 OK");
  } else {
    LOG_W("I2C", "INA3221 non détecté");
  }

  if (mppt.begin()) {
    LOG_I("I2C", "MPPT charger OK");
  } else {
    LOG_W("I2C", "MPPT non détecté");
  }

  if (bme280.begin()) {
    LOG_I("I2C", "BME280 OK");
  } else {
    LOG_W("I2C", "BME280 non détecté");
  }

  if (eeprom.isInitialized()) {
    if (telemetry_history.begin()) {
      LOG_I("I2C", "Historique EEPROM: %d slots", telemetry_history.getMaxRecords());
    }
    if (event_log.begin()) {
      LOG_I("I2C", "Log événements EEPROM: %d slots", event_log.getMaxRecords());
    }
  }

  if (victron.begin()) {
    LOG_I("VICTRON", "VE.Direct OK");
  } else {
    LOG_W("VICTRON", "VE.Direct non détecté");
  }

  // Un seul chargeur solaire à la fois selon la carte : MPPT prioritaire
  // s'il répond, sinon Victron.
  if (mppt.isInitialized()) {
    active_charger = &mppt;
  } else if (victron.isInitialized()) {
    active_charger = &victron;
  } else {
    LOG_W("I2C", "Aucun chargeur solaire détecté (ni MPPT, ni Victron)");
  }
}

void bootInitRelays() {
  if (!modeIsFull(settings.system.mode)) {
    return;
  }
  relay_hal.begin(settings.relay, RELAY_COUNT);
}

void bootInitMeshAndAprs() {
  uint8_t mode = settings.system.mode;

  if (modeHasMeshcore(mode)) {
    sensors.begin();
    the_mesh.begin(&LittleFS);
    // Canaux de groupe (nom/région, cf. config/Settings.h: MeshChannelSettings)
    // depuis les settings — après begin() pour ne pas être écrasés par lui.
    the_mesh.loadChannelsFromSettings(settings);
    the_mesh.sendSelfAdvertisement(16000, false);
  }

  if (modeHasAprs(mode)) {
    // Reconfigure la radio APRS depuis les vrais settings (radio.aprs.freq/
    // bw/sf/cr/power) : bootInitRadios() l'a initialisée plus tôt avec les
    // defines de compile-time (APRS_FREQ/...), settings pas encore chargés à
    // cet instant (cf. main.cpp:setup(), cette étape-ci s'exécute après
    // bootLoadConfig()). switchToLora() fait exactement ce re-begin() à
    // partir des settings — même fonction que celle utilisée au retour d'un
    // cycle FSK/CW/SSTV, donc garantie cohérente avec eux.
    LoRa433RadioMode::switchToLora();

    aprs_dispatcher.begin();
    aprs_dispatcher.setRxCallback(&aprs_engine);
    aprs_engine.setEventCallback(&aprs_event_handler);
  }
}

void bootCreateTasks() {
  uint8_t mode = settings.system.mode;
  LOG_I("RTOS", "Création des tasks (mode=%s)...", modeName(mode));

  if (modeHasMeshcore(mode)) {
    xTaskCreate(taskMeshLoop, "mesh", TASK_STACK_MESH, nullptr, TASK_PRIO_MESH_LOOP, nullptr);
  }

  if (modeHasAprs(mode)) {
    xTaskCreate(taskAprsLoop,   "aprs",    TASK_STACK_APRS,    nullptr, TASK_PRIO_APRS_LOOP, nullptr);
    xTaskCreate(taskAprsBeacon, "beacon",  TASK_STACK_BEACON,  nullptr, TASK_PRIO_BEACON,    nullptr);
    xTaskCreate(taskWeather,    "weather", TASK_STACK_WEATHER, nullptr, TASK_PRIO_WEATHER,   nullptr);
    xTaskCreate(taskSstv,       "sstv",    TASK_STACK_SSTV,    nullptr, TASK_PRIO_SSTV,      nullptr);
  }

  if (modeIsFull(mode)) {
    xTaskCreate(taskSensors, "sensors", TASK_STACK_SENSORS, nullptr, TASK_PRIO_SENSORS, nullptr);
    xTaskCreate(taskEnergy,  "energy",  TASK_STACK_ENERGY,  nullptr, TASK_PRIO_ENERGY,  nullptr);
  }

  // Toujours créées, quel que soit le mode : configuration/diagnostic
  // (série) et watchdog matériel restent utiles en standalone.
  xTaskCreate(taskCli,      "cli", TASK_STACK_CLI,      nullptr, TASK_PRIO_CLI,      nullptr);
  xTaskCreate(taskWatchdog, "wdt", TASK_STACK_WATCHDOG, nullptr, TASK_PRIO_WATCHDOG, nullptr);

  LOG_I("RTOS", "Scheduler démarré");
}

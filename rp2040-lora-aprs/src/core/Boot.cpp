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

// ============================================================================
// Objets globaux déclarés dans main.cpp (racine de composition) — mêmes
// externs que ceux utilisés par tasks/task_*.cpp.
// ============================================================================
extern LogLevel g_log_level;
extern StdRNG fast_rng;
extern Settings settings;
extern MyMesh the_mesh;
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

// ============================================================================
// Étapes de boot
// ============================================================================

void bootInitCore() {
  Serial.begin(115200);
  delay(2000); // attendre USB serial

  board.begin();
  LittleFS.begin();
}

void bootInitRadios() {
  if (!mesh_radio_init()) {
    LOG_E("RADIO", "Init radio 868 FAIL");
  } else {
    LOG_I("RADIO", "Init radio 868 OK");
  }

  if (!aprs_radio_init()) {
    LOG_E("RADIO", "Init radio 433 FAIL");
  } else {
    LOG_I("RADIO", "Init radio 433 OK");
  }
}

void bootSeedRng() {
  fast_rng.begin(radio_get_rng_seed());
}

void bootInitIdentity() {
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

  Serial.print("Repeater ID: ");
  mesh::Utils::printHex(Serial, the_mesh.self_id.pub_key, PUB_KEY_SIZE);
  Serial.println();
}

void bootInitSensors() {
  i2c_bus.begin();
  LOG_I("I2C", "Bus initialisé");

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

  if (eeprom.begin()) {
    LOG_I("I2C", "EEPROM M24M01 OK");
    if (telemetry_history.begin()) {
      LOG_I("I2C", "Historique EEPROM: %d slots", telemetry_history.getMaxRecords());
    }
    if (event_log.begin()) {
      LOG_I("I2C", "Log événements EEPROM: %d slots", event_log.getMaxRecords());
    }
  } else {
    LOG_W("I2C", "EEPROM non détectée");
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

void bootLoadConfig() {
  settings = settings_manager.load();
  settings_registry.init(settings);
  g_log_level = (LogLevel)settings.system.log_level;
  LOG_I("CONFIG", "%d paramètres, log=%s", settings_registry.count(), logLevelName(g_log_level));
}

void bootInitRelays() {
  relay_hal.begin(settings.relay, RELAY_COUNT);
}

void bootInitMeshAndAprs() {
  sensors.begin();
  the_mesh.begin(&LittleFS);

  aprs_dispatcher.begin();
  aprs_dispatcher.setRxCallback(&aprs_engine);
  aprs_engine.setEventCallback(&aprs_event_handler);

  the_mesh.sendSelfAdvertisement(16000, false);
}

void bootCreateTasks() {
  LOG_I("RTOS", "Création des tasks...");

  xTaskCreate(taskMeshLoop,   "mesh",    TASK_STACK_MESH,    nullptr, TASK_PRIO_MESH_LOOP, nullptr);
  xTaskCreate(taskAprsLoop,   "aprs",    TASK_STACK_APRS,    nullptr, TASK_PRIO_APRS_LOOP, nullptr);
  xTaskCreate(taskAprsBeacon, "beacon",  TASK_STACK_BEACON,  nullptr, TASK_PRIO_BEACON,    nullptr);
  xTaskCreate(taskSensors,    "sensors", TASK_STACK_SENSORS, nullptr, TASK_PRIO_SENSORS,   nullptr);
  xTaskCreate(taskEnergy,     "energy",  TASK_STACK_ENERGY,  nullptr, TASK_PRIO_ENERGY,    nullptr);
  xTaskCreate(taskWeather,    "weather", TASK_STACK_WEATHER, nullptr, TASK_PRIO_WEATHER,   nullptr);
  xTaskCreate(taskCli,        "cli",     TASK_STACK_CLI,     nullptr, TASK_PRIO_CLI,       nullptr);
  xTaskCreate(taskWatchdog,   "wdt",     TASK_STACK_WATCHDOG, nullptr, TASK_PRIO_WATCHDOG, nullptr);

  LOG_I("RTOS", "Scheduler démarré");
}

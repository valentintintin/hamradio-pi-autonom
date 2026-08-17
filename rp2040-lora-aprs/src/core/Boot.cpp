#include "Boot.h"

#include <Arduino.h>
#include <LittleFS.h>

#include "target.h"
#include "tasks/tasks.h"

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
#include "aprs/SstvTransmitter.h"

#ifdef NATIVE_BUILD
#include "SimWebBridge.h"
#endif

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

void bootInitCore() {
  Serial.begin(115200);
  delay(2000); // laisse le temps à l'USB CDC de s'énumérer avant les premiers logs

  board.begin();
  LittleFS.begin();

#ifdef NATIVE_BUILD
  startSimWebBridge();
#endif
}

// Toujours exécuté avant que le mode soit connu : bootLoadConfig() peut retomber sur l'EEPROM si LittleFS est vide/corrompu.
void bootInitEeprom() {
  i2c_bus.begin();
  LOG_I("I2C", "Bus initialisé");

  if (eeprom.begin()) {
    LOG_I("I2C", "EEPROM M24M01 OK");
  } else {
    LOG_W("I2C", "EEPROM non détectée");
  }
}

bool bootInitRtc() {
  if (!externalRtc.begin()) {
    LOG_W("RTC", "Puce RX8025T non détectée sur le bus I2C");
    return false;
  }

  uint32_t chipTime;
  if (!externalRtc.readTime(&chipTime)) {
    LOG_W("RTC", "Puce RX8025T présente mais heure non fiable (VLF) — horloge RP2040 non synchronisée");
    return false;
  }

  rtc_clock.setCurrentTime(chipTime);
  LOG_I("RTC", "Horloge RP2040 synchronisée depuis la puce RX8025T");
  return true;
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

// RNG initialisé depuis le bruit RF de la radio 868 (cf. variant/target.cpp: radio_get_rng_seed).
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
    the_mesh.self_id = radio_new_identity();
    int count = 0;
    while (count < 10 && (the_mesh.self_id.pub_key[0] == 0x00 || the_mesh.self_id.pub_key[0] == 0xFF)) {  // 0x00/0xFF réservés par le protocole MeshCore, à éviter
      the_mesh.self_id = radio_new_identity();
      count++;
    }
    store.save("_main", the_mesh.self_id);
  }

  Serial.println("Repeater ID: ");
  mesh::Utils::printHex(Serial, the_mesh.self_id.pub_key, PUB_KEY_SIZE);
  Serial.println();
}

void bootInitSensors() {
  if (!modeHasTelemetry(settings.system.mode)) {
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
      telemetry_history.setEnabled(settings.system.telemetry_log_enabled);
      LOG_I("I2C", "Historique EEPROM: %d slots", telemetry_history.getMaxRecords());
    }
    if (event_log.begin()) {
      event_log.setEnabled(settings.system.event_log_enabled);
      LOG_I("I2C", "Log événements EEPROM: %d slots", event_log.getMaxRecords());
    }
  }

  if (victron.begin()) {
    LOG_I("VICTRON", "VE.Direct OK");
  } else {
    LOG_W("VICTRON", "VE.Direct non détecté");
  }

  // Un seul chargeur actif à la fois : MPPT prioritaire sur Victron.
  if (mppt.isInitialized()) {
    active_charger = &mppt;
  } else if (victron.isInitialized()) {
    active_charger = &victron;
  } else {
    LOG_W("I2C", "Aucun chargeur solaire détecté (ni MPPT, ni Victron)");
  }
}

void bootInitRelays() {
  if (!modeHasTelemetry(settings.system.mode)) {
    return;
  }
  relay_hal.begin(RELAY_COUNT);
}

void bootInitMeshAndAprs() {
  uint8_t mode = settings.system.mode;

  if (modeHasMeshcore(mode)) {
    sensors.begin();
    the_mesh.begin(&LittleFS);
    the_mesh.loadChannelsFromSettings(settings); // après begin() pour ne pas être écrasé par lui
    the_mesh.sendSelfAdvertisement(16000, false);
  }

  if (modeHasAprs(mode)) {
    // Re-init radio depuis les settings réels : bootInitRadios() l'a démarrée avec les defines de compile-time.
    aprs_radio_hw.switchToLora();

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

  if (modeHasTelemetry(mode)) {
    xTaskCreate(taskSensors, "sensors", TASK_STACK_SENSORS, nullptr, TASK_PRIO_SENSORS, nullptr);
    xTaskCreate(taskEnergy,  "energy",  TASK_STACK_ENERGY,  nullptr, TASK_PRIO_ENERGY,  nullptr);
  }

  xTaskCreate(taskCli,      "cli", TASK_STACK_CLI,      nullptr, TASK_PRIO_CLI,      nullptr);
  xTaskCreate(taskWatchdog, "wdt", TASK_STACK_WATCHDOG, nullptr, TASK_PRIO_WATCHDOG, nullptr);

  LOG_I("RTOS", "Scheduler démarré");
}

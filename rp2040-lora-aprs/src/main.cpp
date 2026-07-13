// ============================================================================
// RP-LoRA_Mini_v3_dual — main.cpp
//
// Dual SX1262 : MeshCore (868 MHz, SPI1) + APRS (433 MHz, SPI0)
// FreeRTOS sur RP2040 (Pico W, earlephilhower core)
// ============================================================================

#include <Arduino.h>
#include <LittleFS.h>

#include "target.h"
#include "tasks/tasks.h"

// MeshCore core
#include <helpers/SimpleMeshTables.h>
#include <helpers/ArduinoHelpers.h>
#include <helpers/IdentityStore.h>

// Nos modules
#include "mesh/MeshcoreRepeater.h"
#include "aprs/AprsDispatcher.h"
#include "aprs/AprsEngine.h"
#include "aprs/AprsEventHandler.h"
#include "hal/Telemetry.h"
#include "hal/I2CBus.h"
#include "hal/Ina3221Hal.h"
#include "hal/MpptChargerHal.h"
#include "hal/Bme280Hal.h"
#include "hal/VictronHal.h"
#include "hal/TelemetryHistory.h"
#include "hal/RelayHal.h"
#include "config/Log.h"
#include "config/Settings.h"
#include "config/SettingsManager.h"
#include "config/SettingsRegistry.h"
#include "config/CommandHandler.h"

// ============================================================================
// Instances globales
// ============================================================================

// Log level global (synchronisé avec settings.system.log_level)
LogLevel g_log_level = LOG_INFO;

// Horloge + RNG
static ArduinoMillis ms_clock;
static StdRNG fast_rng;

// MeshCore
static SimpleMeshTables mesh_tables;
MyMesh the_mesh(board, mesh_radio_driver, ms_clock, fast_rng, rtc_clock, mesh_tables);

// APRS
AprsDispatcher aprs_dispatcher(aprs_radio_driver, ms_clock);
static AprsConfig aprs_config = {};
AprsEngine aprs_engine(aprs_dispatcher, aprs_config);

// Telemetry partagée (lue par beacon, bridge, CLI)
TelemetryData telemetry = {};

// I2C bus + HAL capteurs
I2CBus i2c_bus(Wire, 4, 5);  // SDA=GP4, SCL=GP5 — I2C0
Ina3221Hal ina3221(i2c_bus);
MpptChargerHal mppt(i2c_bus);
Bme280Hal bme280(i2c_bus);
VictronHal victron(Serial1);  // VE.Direct sur UART1
EepromHal eeprom(i2c_bus);

// Historique télémétrie EEPROM
TelemetryHistory telemetry_history(eeprom);

// Relais bistables (carte Interface F1ZIC, expandeur TCA9555 @0x20 sur I2C0)
RelayHal relay_hal(i2c_bus);

// Configuration
Settings settings;
SettingsManager settings_manager(&eeprom);
SettingsRegistry settings_registry;
CommandHandler command_handler(settings, settings_registry, settings_manager, telemetry,
                               aprs_engine, relay_hal, &telemetry_history);

// Relie AprsEngine à la télémétrie et au CLI (query météo, telemetry, CLI par message)
AprsEventHandler aprs_event_handler(aprs_engine, command_handler, telemetry, settings);

// ============================================================================
// Helpers — copie config APRS depuis settings
// ============================================================================
static void applyAprsConfig() {
  strncpy(aprs_config.callsign, settings.aprs.callsign, sizeof(aprs_config.callsign));
  strncpy(aprs_config.destination, settings.aprs.destination, sizeof(aprs_config.destination));
  strncpy(aprs_config.path, settings.aprs.path, sizeof(aprs_config.path));
  strncpy(aprs_config.pathTelemetry, settings.aprs.pathTelemetry, sizeof(aprs_config.pathTelemetry));
  strncpy(aprs_config.comment, settings.aprs.comment, sizeof(aprs_config.comment));
  aprs_config.symbol = settings.aprs.symbol;
  aprs_config.symbolTable = settings.aprs.symbolTable;
  aprs_config.latitude = settings.aprs.latitude;
  aprs_config.longitude = settings.aprs.longitude;
  aprs_config.altitude = settings.aprs.altitude;
  aprs_config.digipeaterEnabled = settings.aprs.digipeaterEnabled;
  aprs_config.telemetrySequenceNumber = 0;
}

// ============================================================================
// Setup
// ============================================================================
void setup() {
  Serial.begin(115200);
  delay(2000); // attendre USB serial

  // Board init
  board.begin();

  // Filesystem
  LittleFS.begin();

  // --- Init radio MeshCore (868 MHz, SPI1) ---------------------------------
  if (!mesh_radio_init()) LOG_E("RADIO", "Init radio 868 FAIL");
  else                    LOG_I("RADIO", "Init radio 868 OK");

  // --- Init radio APRS (433 MHz, SPI0) ------------------------------------
  if (!aprs_radio_init()) LOG_E("RADIO", "Init radio 433 FAIL");
  else                    LOG_I("RADIO", "Init radio 433 OK");

  // --- RNG — seed depuis bruit radio ---------------------------------------
  fast_rng.begin(radio_get_rng_seed());

  // --- Identity MeshCore — charge ou génère --------------------------------
  {
    IdentityStore store(LittleFS, "/identity");
    store.begin();

    if (!store.load("_main", the_mesh.self_id)) {
      LOG_W("MESH", "Génération nouvelle identité");
      the_mesh.self_id = radio_new_identity();   // create new random identity
      int count = 0;
      while (count < 10 && (the_mesh.self_id.pub_key[0] == 0x00 || the_mesh.self_id.pub_key[0] == 0xFF)) {  // reserved id hashes
        the_mesh.self_id = radio_new_identity(); count++;
      }
      store.save("_main", the_mesh.self_id);
    }

    Serial.print("Repeater ID: ");
    mesh::Utils::printHex(Serial, the_mesh.self_id.pub_key, PUB_KEY_SIZE); Serial.println();
  }

  // --- Init I2C bus + capteurs ----------------------------------------------
  i2c_bus.begin();
  LOG_I("I2C", "Bus initialisé");

  if (ina3221.begin()) LOG_I("I2C", "INA3221 OK");
  else                 LOG_W("I2C", "INA3221 non détecté");

  if (mppt.begin())    LOG_I("I2C", "MPPT charger OK");
  else                 LOG_W("I2C", "MPPT non détecté");

  if (bme280.begin())  LOG_I("I2C", "BME280 OK");
  else                 LOG_W("I2C", "BME280 non détecté");

  if (eeprom.begin()) {
    LOG_I("I2C", "EEPROM M24M01 OK");
    if (telemetry_history.begin())
      LOG_I("I2C", "Historique EEPROM: %d slots", telemetry_history.getMaxRecords());
  } else {
    LOG_W("I2C", "EEPROM non détectée");
  }

  if (victron.begin()) LOG_I("VICTRON", "VE.Direct OK");
  else                 LOG_W("VICTRON", "VE.Direct non détecté");

  // --- Charger la configuration --------------------------------------------
  settings = settings_manager.load();
  settings_registry.init(settings);
  g_log_level = (LogLevel)settings.system.log_level;
  LOG_I("CONFIG", "%d paramètres, log=%s", settings_registry.count(), logLevelName(g_log_level));

  // --- Appliquer la config APRS --------------------------------------------
  applyAprsConfig();

  // --- Relais bistables (broches configurées via CLI, cf settings.relay) --
  relay_hal.begin(settings.relay, RELAY_COUNT);

  // --- Init MeshCore -------------------------------------------------------
  the_mesh.begin(&LittleFS);

  // --- Init APRS -----------------------------------------------------------
  aprs_dispatcher.begin();
  aprs_dispatcher.setRxCallback(&aprs_engine);
  aprs_engine.setEventCallback(&aprs_event_handler);

  // --- Advert initial ------------------------------------------------------
  the_mesh.sendSelfAdvertisement(16000, false);

  // --- Créer les tasks FreeRTOS --------------------------------------------
  LOG_I("RTOS", "Création des tasks...");

  xTaskCreate(taskMeshLoop,   "mesh",    TASK_STACK_MESH,    nullptr, TASK_PRIO_MESH_LOOP, nullptr);
  xTaskCreate(taskAprsLoop,   "aprs",    TASK_STACK_APRS,    nullptr, TASK_PRIO_APRS_LOOP, nullptr);
  xTaskCreate(taskAprsBeacon, "beacon",  TASK_STACK_BEACON,  nullptr, TASK_PRIO_BEACON,    nullptr);
  xTaskCreate(taskEnergy,     "energy",  TASK_STACK_ENERGY,  nullptr, TASK_PRIO_ENERGY,    nullptr);
  xTaskCreate(taskWeather,    "weather", TASK_STACK_WEATHER, nullptr, TASK_PRIO_WEATHER,   nullptr);
  xTaskCreate(taskCli,        "cli",     TASK_STACK_CLI,     nullptr, TASK_PRIO_CLI,       nullptr);

  LOG_I("RTOS", "Scheduler démarré");

  board.onBootComplete();
}

// ============================================================================
// Loop — vide avec FreeRTOS, tout est dans les tasks
// ============================================================================
void loop() {
  vTaskDelay(pdMS_TO_TICKS(1000));
}

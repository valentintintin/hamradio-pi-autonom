#include <Arduino.h>
#include <LittleFS.h>

#include "target.h"
#include "tasks/tasks.h"

#include <helpers/SimpleMeshTables.h>
#include <helpers/ArduinoHelpers.h>

#include "mesh/MeshcoreRepeater.h"
#include "aprs/AprsDispatcher.h"
#include "aprs/AprsEngine.h"
#include "aprs/AprsEventHandler.h"
#include "hal/Telemetry.h"
#include "hal/i2c/I2CBus.h"
#include "hal/sensors/Ina3221Hal.h"
#include "hal/chargers/MpptChargerHal.h"
#include "hal/sensors/Bme280Hal.h"
#include "hal/chargers/VictronHal.h"
#include "hal/chargers/ChargeControllerHal.h"
#include "hal/eeprom/TelemetryHistory.h"
#include "hal/eeprom/EventLogHistory.h"
#include "hal/gpio/Tca9555Hal.h"
#include "hal/relay/RelayHal.h"
#include "hal/relay/Relay.h"
#include "energy/MpptShutdownMonitor.h"
#include "core/Log.h"
#include "core/Boot.h"
#include "config/Settings.h"
#include "config/SettingsManager.h"
#include "config/SettingsRegistry.h"
#include "cli/CommandHandler.h"
#include "aprs/SstvTransmitter.h"

LogLevel g_log_level = LOG_INFO;

static ArduinoMillis ms_clock;
StdRNG fast_rng;

Settings settings;

static SimpleMeshTables mesh_tables;
MeshcoreRepeater the_mesh(board, mesh_radio_driver, ms_clock, fast_rng, rtc_clock, mesh_tables);

AprsDispatcher aprs_dispatcher(aprs_radio_driver, ms_clock);
AprsEngine aprs_engine(aprs_dispatcher, settings.aprs);

TelemetryData telemetry = {};

I2CBus i2c_bus(Wire, 4, 5);
Ina3221Hal ina3221(i2c_bus);
MpptChargerHal mppt(i2c_bus);
Bme280Hal bme280(i2c_bus);
VictronHal victron(Serial1);
M24M01Hal eeprom(i2c_bus);

// Un seul des deux (MPPT I2C / Victron VE.Direct) est réellement présent ;
// déterminé au boot (bootInitSensors), nullptr si aucun détecté.
ChargeControllerHal* active_charger = nullptr;

TelemetryHistory telemetry_history(eeprom);
EventLogHistory event_log(eeprom);

Tca9555Hal relay_expander(i2c_bus, TCA9555_RELAY_ADDR);
RelayHal relay_hal(relay_expander);
Relay relays[RELAY_COUNT] = {
  Relay(0, settings.relay[0], relay_hal, event_log),
  Relay(1, settings.relay[1], relay_hal, event_log),
  Relay(2, settings.relay[2], relay_hal, event_log),
  Relay(3, settings.relay[3], relay_hal, event_log),
};

MpptShutdownMonitor mppt_shutdown_monitor(mppt, aprs_engine, event_log);

SettingsManager settings_manager(&eeprom);
SettingsRegistry settings_registry;

SstvTransmitter sstv_transmitter;

CommandHandler command_handler(settings, settings_registry, settings_manager, telemetry,
                               aprs_engine, relays, &telemetry_history, &mppt, &event_log,
                               &sstv_transmitter, &the_mesh);

AprsEventHandler aprs_event_handler(aprs_engine, command_handler, telemetry, settings, relays);

void setup() {
  bootInitCore();
  bootInitEeprom();
  bootInitRtc();
  // bootLoadConfig() doit précéder bootInitRadios (et suivantes) : avant
  // elle, settings.system.mode vaut encore sa valeur zéro-init, pas le mode
  // réellement configuré.
  bootLoadConfig();
  bootInitRadios();
  bootSeedRng();
  bootInitIdentity();
  bootInitSensors();
  bootInitRelays();
  bootInitMeshAndAprs();
  bootCreateTasks();

  board.onBootComplete();
}

void loop() {
  vTaskDelay(pdMS_TO_TICKS(1000));
}

// ============================================================================
// RP-LoRA_Mini_v3_dual — main.cpp
//
// Dual SX1262 : MeshCore (868 MHz, SPI1) + APRS (433 MHz, SPI0)
// FreeRTOS sur RP2040 (Pico W, earlephilhower core)
//
// Racine de composition de l'appli : tous les objets globaux (radios, bus
// I2C, HAL, contrôleurs énergie, CLI...) sont déclarés ici et référencés par
// extern ailleurs (tasks/task_*.cpp, core/Boot.cpp). setup() ne fait que
// dérouler la séquence de boot (cf. core/Boot.h) — le détail de chaque étape
// vit dans core/Boot.cpp pour rester lisible.
// ============================================================================

#include <Arduino.h>
#include <LittleFS.h>

#include "target.h"
#include "tasks/tasks.h"

// MeshCore core
#include <helpers/SimpleMeshTables.h>
#include <helpers/ArduinoHelpers.h>

// Nos modules
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
#include "hal/i2c/Tca9555Hal.h"
#include "hal/relay/RelayHal.h"
#include "energy/MpptShutdownMonitor.h"
#include "core/Log.h"
#include "core/Boot.h"
#include "config/Settings.h"
#include "config/SettingsManager.h"
#include "config/SettingsRegistry.h"
#include "cli/CommandHandler.h"
#include "aprs/SstvTransmitter.h"

// ============================================================================
// Instances globales
// ============================================================================

// Log level global (synchronisé avec settings.system.log_level)
LogLevel g_log_level = LOG_INFO;

// Horloge + RNG
static ArduinoMillis ms_clock;
StdRNG fast_rng;

// Configuration — déclarée tôt : plusieurs modules (AprsEngine...) référencent
// directement ses sous-structures plutôt que d'en garder une copie locale.
Settings settings;

// MeshCore
static SimpleMeshTables mesh_tables;
MeshcoreRepeater the_mesh(board, mesh_radio_driver, ms_clock, fast_rng, rtc_clock, mesh_tables);

// APRS — AprsEngine référence directement settings.aprs (pas de copie locale),
// donc un "set aprs.xxx" au CLI prend effet immédiatement, sans étape de sync.
AprsDispatcher aprs_dispatcher(aprs_radio_driver, ms_clock);
AprsEngine aprs_engine(aprs_dispatcher, settings.aprs);

// Telemetry partagée (lue par beacon, bridge, CLI)
TelemetryData telemetry = {};

// I2C bus + HAL capteurs
I2CBus i2c_bus(Wire, 4, 5);  // SDA=GP4, SCL=GP5 — I2C0
Ina3221Hal ina3221(i2c_bus);
MpptChargerHal mppt(i2c_bus);
Bme280Hal bme280(i2c_bus);
VictronHal victron(Serial1);  // VE.Direct sur UART1
M24M01Hal eeprom(i2c_bus);

// Un seul chargeur solaire présent à la fois selon la révision de carte
// (MPPT I2C ou Victron VE.Direct) — déterminé au boot (cf. core/Boot.cpp:
// bootInitSensors) une fois les deux begin() tentés. nullptr si aucun des
// deux n'est détecté.
ChargeControllerHal* active_charger = nullptr;

// Historique télémétrie EEPROM
TelemetryHistory telemetry_history(eeprom);

// Log d'événements critiques EEPROM (code + données numériques, cf.
// hal/eeprom/EventLogHistory.h) — journalisé explicitement par ses sites d'origine
// (task_watchdog.cpp, energy/Relay.cpp, energy/MpptShutdownMonitor.cpp)
EventLogHistory event_log(eeprom);

// Relais bistables (carte Interface F1ZIC, expandeur TCA9555 @0x20 sur I2C0)
Tca9555Hal relay_expander(i2c_bus, TCA9555_RELAY_ADDR);
RelayHal relay_hal(relay_expander);

// Supervision énergie (cf. src/energy/, orchestrée par task_energy.cpp)
MpptShutdownMonitor mppt_shutdown_monitor(mppt, aprs_engine, event_log);

// Configuration (settings elle-même déclarée plus haut, cf. commentaire)
SettingsManager settings_manager(&eeprom);
SettingsRegistry settings_registry;

// Upload d'image (streaming LittleFS) + émission CW+SSTV (cf. aprs/SstvTransmitter.h,
// tasks/task_sstv.cpp) — settings.cw_sstv, référencée directement.
SstvTransmitter sstv_transmitter;

CommandHandler command_handler(settings, settings_registry, settings_manager, telemetry,
                               aprs_engine, relay_hal, &telemetry_history, &mppt, &event_log,
                               &sstv_transmitter, &the_mesh);

// Relie AprsEngine à la télémétrie et au CLI (query météo, telemetry, CLI par message)
AprsEventHandler aprs_event_handler(aprs_engine, command_handler, telemetry, settings, relay_hal);

// ============================================================================
// Setup — la séquence détaillée vit dans core/Boot.cpp, une fonction par
// étape (ordre important : ex. bootInitRelays dépend des settings chargés
// par bootLoadConfig).
// ============================================================================
void setup() {
  bootInitCore();
  bootInitEeprom();
  // bootLoadConfig() doit précéder toute étape qui lit settings.system.mode
  // (bootInitRadios et les suivantes) : avant elle, settings est encore à sa
  // valeur zéro-initialisée (MODE_APRS_ONLY), pas le mode réellement
  // configuré — cf. commentaire de core/Boot.h sur "le mode réellement
  // configuré".
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

// ============================================================================
// Loop — vide avec FreeRTOS, tout est dans les tasks
// ============================================================================
void loop() {
  vTaskDelay(pdMS_TO_TICKS(1000));
}

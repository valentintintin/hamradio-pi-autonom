#pragma once

#include <stdint.h>
#include <cstring>

// ============================================================================
// Settings — struct de configuration persistante
//
// Stockée en binaire sur LittleFS (/settings.bin) avec fallback EEPROM.
// Un magic number + version permet de détecter une config invalide
// et de charger les valeurs par défaut.
// ============================================================================

#define SETTINGS_MAGIC    0x46345345  // "F4SE"
#define SETTINGS_VERSION  1

struct AprsSettings {
  char callsign[10];        // ex: "F4ISE-10"
  char destination[7];      // ex: "APRS"
  char path[16];            // ex: "WIDE1-1"
  char pathTelemetry[16];   // ex: "" (pas de path pour telemetry)
  char symbol;              // ex: '#'
  char symbolTable;         // ex: '/'
  float latitude;
  float longitude;
  int16_t altitude;         // mètres
  bool digipeaterEnabled;
  uint32_t intervalPosition_ms;   // beacon position (2x/jour)
  uint32_t intervalTelemetry_ms;  // beacon telemetry (toutes les heures)
  uint32_t intervalWeather_ms;    // beacon météo (toutes les 15 min)
  uint32_t intervalStatus_ms;     // filet de sécurité statut (envoi forcé même si état inchangé)
  char comment[32];               // commentaire APRS
};

struct RadioSettings {
  // 433 APRS
  float aprs_freq;
  float aprs_bw;
  uint8_t aprs_sf;
  uint8_t aprs_cr;
  int8_t aprs_tx_power;
  // 868 Mesh (lecture seule pour info, pas modifiable à chaud)
  float mesh_freq;
};

struct WeatherSettings {
  bool wh65b_enabled;
  uint32_t wh65b_interval_ms;    // intervalle switch FSK (10min)
  uint32_t wh65b_rx_timeout_ms;  // timeout écoute (60s)
};

struct EnergySettings {
  uint32_t poll_interval_ms;     // intervalle lecture capteurs
  uint32_t mppt_wdt_interval_ms; // intervalle feed watchdog MPPT
  bool mppt_wdt_enabled;
  bool victron_enabled;
};

struct SystemSettings {
  char admin_password[16];
  bool watchdog_enabled;
  uint32_t telemetry_log_interval_ms;  // intervalle log EEPROM
  uint8_t log_level;                   // LogLevel (0=none .. 5=trace)
};

// Relais bistable piloté via l'expandeur I2C TCA9555 (carte Interface F1ZIC,
// cf. hal/RelayHal.h). Le mapping broche est fixé par le câblage matériel
// (P00-P07 du TCA9555) ; seul l'état logique est persisté ici — le relais
// bistable garde sa position mécanique sans alimentation, même après reboot.
#define RELAY_COUNT       4

struct RelayChannel {
  bool state;
};

struct Settings {
  uint32_t magic;
  uint16_t version;

  AprsSettings aprs;
  RadioSettings radio;
  WeatherSettings weather;
  EnergySettings energy;
  SystemSettings system;
  RelayChannel relay[RELAY_COUNT];
};

// ============================================================================
// Valeurs par défaut
// ============================================================================
inline Settings getDefaultSettings() {
  Settings s = {};

  s.magic = SETTINGS_MAGIC;
  s.version = SETTINGS_VERSION;

  // APRS
  strncpy(s.aprs.callsign, "F4ISE-10", sizeof(s.aprs.callsign));
  strncpy(s.aprs.destination, "APRS", sizeof(s.aprs.destination));
  strncpy(s.aprs.path, "WIDE1-1", sizeof(s.aprs.path));
  s.aprs.pathTelemetry[0] = '\0';
  s.aprs.symbol = '#';
  s.aprs.symbolTable = '/';
  s.aprs.latitude = 0.0f;
  s.aprs.longitude = 0.0f;
  s.aprs.altitude = 0;
  s.aprs.digipeaterEnabled = true;
  s.aprs.intervalPosition_ms = 43200000;   // 12h (2x/jour)
  s.aprs.intervalTelemetry_ms = 3600000;   // 1h
  s.aprs.intervalWeather_ms = 900000;      // 15min
  s.aprs.intervalStatus_ms = 21600000;     // 6h (filet de sécurité, sinon envoi si l'état change)
  strncpy(s.aprs.comment, "LoRa Dual Digi", sizeof(s.aprs.comment));

  // Radio
  s.radio.aprs_freq = 433.775f;
  s.radio.aprs_bw = 125.0f;
  s.radio.aprs_sf = 12;
  s.radio.aprs_cr = 5;
  s.radio.aprs_tx_power = 22;
  s.radio.mesh_freq = 869.618f;

  // Weather
  s.weather.wh65b_enabled = true;
  s.weather.wh65b_interval_ms = 600000;    // 10min
  s.weather.wh65b_rx_timeout_ms = 60000;   // 60s

  // Energy
  s.energy.poll_interval_ms = 30000;       // 30s
  s.energy.mppt_wdt_interval_ms = 90000;   // 90s
  s.energy.mppt_wdt_enabled = true;
  s.energy.victron_enabled = true;

  // System
  strncpy(s.system.admin_password, "password", sizeof(s.system.admin_password));
  s.system.watchdog_enabled = false;
  s.system.telemetry_log_interval_ms = 300000; // 5min
  s.system.log_level = 3;                      // INFO par défaut

  // Relais — tous désactivés par défaut
  for (int i = 0; i < RELAY_COUNT; i++) {
    s.relay[i].state = false;
  }

  return s;
}

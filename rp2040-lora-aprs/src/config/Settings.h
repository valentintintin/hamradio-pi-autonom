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

#define SETTINGS_MAGIC    0x34485656  // "4HVV"
#define SETTINGS_VERSION  5

// ============================================================================
// Mode de fonctionnement — la carte peut être déployée en standalone (juste
// un digipeater APRS, ou juste un répéteur MeshCore) ou complète (station
// solaire avec relais + télémétrie). Détermine quelles tâches FreeRTOS sont
// créées et quels sous-systèmes sont initialisés (cf. core/Boot.cpp).
// Numérique uniquement côté CLI ("set system.mode 3"), comme system.log_level
// — modifiable à chaud dans les settings, mais ne prend effet qu'au reboot
// (la création des tâches ne se fait qu'une fois, dans setup()).
// ============================================================================
enum OperatingMode : uint8_t {
  MODE_APRS_ONLY     = 0,
  MODE_MESHCORE_ONLY = 1,
  MODE_APRS_MESHCORE = 2,
  MODE_FULL          = 3,  // APRS + MeshCore + relais + télémétrie (fonctionnement normal)
};

inline bool modeHasAprs(uint8_t mode) {
  return mode == MODE_APRS_ONLY || mode == MODE_APRS_MESHCORE || mode == MODE_FULL;
}
inline bool modeHasMeshcore(uint8_t mode) {
  return mode == MODE_MESHCORE_ONLY || mode == MODE_APRS_MESHCORE || mode == MODE_FULL;
}
inline bool modeIsFull(uint8_t mode) {
  return mode == MODE_FULL;
}
inline const char* modeName(uint8_t mode) {
  switch (mode) {
    case MODE_APRS_ONLY:     return "aprs";
    case MODE_MESHCORE_ONLY: return "meshcore";
    case MODE_APRS_MESHCORE: return "aprs+meshcore";
    case MODE_FULL:          return "full";
    default:                 return "?";
  }
}

struct AprsSettings {
  char callsign[10];        // ex: "F4HVV-15"
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
  bool low_voltage_cutoff_enabled; // active la supervision relay_cutoff[]

  // Seuils de coupure/reprise matériels de la carte MPPT elle-même (registres
  // CFG_PWR_OFF_TH / CFG_PWR_ON_TH, cf. lib/mpptChg) — indépendants de la
  // coupure logicielle par relais ci-dessus. 0 = ne pas toucher au réglage
  // usine de la carte (poussé au chip seulement si > 0, cf. task_energy.cpp).
  uint16_t mppt_pwr_off_mv;
  uint16_t mppt_pwr_on_mv;
};

struct SystemSettings {
  char admin_password[16];
  bool watchdog_enabled;
  uint32_t telemetry_log_interval_ms;  // intervalle log EEPROM
  uint8_t log_level;                   // LogLevel (0=none .. 5=trace)
  uint8_t mode;                        // OperatingMode (0..3, cf. ci-dessus)
};

#define RELAY_COUNT       4

struct RelayChannel {
  bool state;
};

// Règle de coupure basse-tension avec hystérésis réelle : si la tension
// batterie (mini de battery_mppt/battery_ina) descend sous min_voltage_mv,
// le relais relay_number est coupé (cf. task_energy.cpp) ; il est reconnecté
// automatiquement une fois la tension remontée au-dessus de
// restore_voltage_mv (doit être > min_voltage_mv). restore_voltage_mv = 0
// désactive la reconnexion automatique (coupure seule, reconnexion manuelle
// via CLI "relay.N.state").
//
// debounce_ms : la tension doit rester en continu au-delà du seuil (coupure
// ou reprise) pendant cette durée avant que l'action ne soit prise — évite
// qu'une chute de tension transitoire (appel de courant bref) ne déclenche
// une coupure inutile. 0 = action immédiate (pas de confirmation).
#define RELAY_CUTOFF_COUNT RELAY_COUNT

struct RelayCutoffRule {
  uint8_t relay_number;        // 1..RELAY_COUNT, même numérotation que "relay.N.state" ; 0 = règle désactivée
  uint16_t min_voltage_mv;     // coupure en dessous de ce seuil
  uint16_t restore_voltage_mv; // reconnexion automatique au-dessus ; 0 = pas de reconnexion auto
  uint32_t debounce_ms;        // durée de confirmation avant coupure/reprise
};

// Fonction "réveil" périodique par relais : indépendamment de la coupure
// basse-tension, allume le relais pendant on_duration_ms toutes les
// interval_ms (cf. task_energy.cpp). override_low_voltage décide si ce
// réveil a lieu même si la coupure basse-tension maintiendrait le relais
// éteint (true), ou s'il est simplement sauté ce cycle-là (false).
struct RelayPeriodicRule {
  bool enabled;
  uint32_t interval_ms;      // ex: 600000 = toutes les 10 min
  uint32_t on_duration_ms;   // ex: 60000 = allumé 1 min à chaque cycle
  bool override_low_voltage;
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
  RelayCutoffRule relay_cutoff[RELAY_CUTOFF_COUNT];
  RelayPeriodicRule relay_periodic[RELAY_COUNT];
};

// ============================================================================
// Valeurs par défaut
// ============================================================================
inline Settings getDefaultSettings() {
  Settings s = {};

  s.magic = SETTINGS_MAGIC;
  s.version = SETTINGS_VERSION;

  // APRS
  strncpy(s.aprs.callsign, "F4HVV-15", sizeof(s.aprs.callsign));
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
  strncpy(s.aprs.comment, "APRS Digi + Meshcore", sizeof(s.aprs.comment));

  // Radio
  s.radio.aprs_freq = 433.775f;
  s.radio.aprs_bw = 125.0f;
  s.radio.aprs_sf = 12;
  s.radio.aprs_cr = 5;
  s.radio.aprs_tx_power = 22;

  // Weather
  s.weather.wh65b_enabled = true;
  s.weather.wh65b_interval_ms = 600000;    // 10min
  s.weather.wh65b_rx_timeout_ms = 60000;   // 60s

  // Energy
  s.energy.poll_interval_ms = 30000;       // 30s
  s.energy.mppt_wdt_interval_ms = 90000;   // 90s
  s.energy.mppt_wdt_enabled = true;
  s.energy.low_voltage_cutoff_enabled = false;
  s.energy.mppt_pwr_off_mv = 0;
  s.energy.mppt_pwr_on_mv = 0;

  // System
  strncpy(s.system.admin_password, "hvv", sizeof(s.system.admin_password));
  s.system.watchdog_enabled = false;
  s.system.telemetry_log_interval_ms = 300000; // 5min
  s.system.log_level = 3;                      // INFO par défaut
  s.system.mode = MODE_FULL;                   // fonctionnement normal par défaut

  // Relais — tous désactivés par défaut
  for (auto & [state] : s.relay) {
    state = false;
  }

  // Coupure basse-tension — désactivée par défaut (relay_number = 0)
  for (auto& rule : s.relay_cutoff) {
    rule.relay_number = 0;
    rule.min_voltage_mv = 0;
    rule.restore_voltage_mv = 0;
    rule.debounce_ms = 60000;
  }

  // Réveil périodique — désactivé par défaut
  for (auto& rule : s.relay_periodic) {
    rule.enabled = false;
    rule.interval_ms = 600000;   // 10 min (inerte tant que enabled=false)
    rule.on_duration_ms = 60000; // 1 min
    rule.override_low_voltage = false;
  }

  return s;
}

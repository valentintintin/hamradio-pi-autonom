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
#define SETTINGS_VERSION  8

// ============================================================================
// Mode de fonctionnement — la carte peut être déployée en standalone (juste
// un digipeater APRS, ou juste un répéteur MeshCore), complète (station
// solaire avec relais + télémétrie), ou en station de télémétrie seule
// (relais + télémétrie, sans radio APRS/MeshCore). Détermine quelles tâches
// FreeRTOS sont créées et quels sous-systèmes sont initialisés (cf.
// core/Boot.cpp).
// Numérique uniquement côté CLI ("set system.mode 3"), comme system.log_level
// — modifiable à chaud dans les settings, mais ne prend effet qu'au reboot
// (la création des tâches ne se fait qu'une fois, dans setup()).
// ============================================================================
enum OperatingMode : uint8_t {
  MODE_APRS_ONLY      = 0,
  MODE_MESHCORE_ONLY  = 1,
  MODE_APRS_MESHCORE  = 2,
  MODE_FULL           = 3,  // APRS + MeshCore + relais + télémétrie (fonctionnement normal)
  MODE_TELEMETRY_ONLY = 4,  // relais + télémétrie seuls, sans APRS ni MeshCore (pas de radio)
};

inline bool modeHasAprs(uint8_t mode) {
  return mode == MODE_APRS_ONLY || mode == MODE_APRS_MESHCORE || mode == MODE_FULL;
}
inline bool modeHasMeshcore(uint8_t mode) {
  return mode == MODE_MESHCORE_ONLY || mode == MODE_APRS_MESHCORE || mode == MODE_FULL;
}
// Relais + télémétrie (capteurs/chargeurs/historique EEPROM) : MODE_FULL et
// MODE_TELEMETRY_ONLY (qui n'en diffère que par l'absence d'APRS/MeshCore).
inline bool modeHasTelemetry(uint8_t mode) {
  return mode == MODE_FULL || mode == MODE_TELEMETRY_ONLY;
}
inline const char* modeName(uint8_t mode) {
  switch (mode) {
    case MODE_APRS_ONLY:      return "aprs";
    case MODE_MESHCORE_ONLY:  return "meshcore";
    case MODE_APRS_MESHCORE:  return "aprs+meshcore";
    case MODE_FULL:           return "full";
    case MODE_TELEMETRY_ONLY: return "telemetry";
    default:                  return "?";
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
  bool aprs_cad_enabled;
};

struct WeatherSettings {
  bool wh65b_enabled;
  uint32_t wh65b_interval_ms;    // intervalle switch FSK (10min)
  uint32_t wh65b_rx_timeout_ms;  // timeout écoute (60s)

  // Relais RF brut d'une trame WH65B décodée avec succès : retransmission des
  // mêmes octets bruts, en FSK, sur la même fréquence, après un délai — pour
  // les autres stations/récepteurs WH65B à portée (pas une conversion APRS,
  // cf. tasks/task_weather.cpp).
  bool resend_enabled;
  int8_t resend_power_dbm;    // ex: 10
  uint32_t resend_delay_ms;   // ex: 5000
};

// Réglages CW (identification morse) + SSTV (envoi d'image), radio 433 APRS.
// sstv_mode est réglé par nom via SettingsRegistry (ST_ENUM8, cf.
// aprs/SstvTransmitter.h pour la table des modes disponibles).
struct CwSstvSettings {
  uint8_t sstv_mode;    // index dans SSTV_MODE_TABLE (aprs/SstvTransmitter.h) — "sstv.mode"
  float freq_mhz;       // fréquence CW+SSTV, ex: 437.000 — "sstv.freq_mhz"
  uint8_t cw_wpm;       // vitesse CW, ex: 20 — "sstv.cw_wpm"
  uint8_t cw_repeats;   // répétitions indicatif avant ET après, ex: 3 — "sstv.cw_repeats"
  int8_t power_dbm;     // puissance CW+SSTV, ex: 22 — "sstv.power_dbm"
};

// Canaux de groupe MeshCore ("mesh.channel.N.name"/"mesh.channel.N.region",
// N = 0..MAX_GROUP_CHANNELS-1 — cf. mesh/MeshcoreRepeater.h). Seuls le nom et
// la région sont persistés : le secret/hash réel du canal est dérivé du nom
// par sha256 au chargement (MeshcoreRepeater::loadChannelsFromSettings/
// add_meshcore_bridge_channel), jamais stocké tel quel ici. name[0]=='\0'
// signifie un slot inutilisé (ignoré au chargement).
struct MeshChannelSettings {
  char name[32];    // "" = inutilisé ; "Public" = canal public MeshCore standard (PSK connue)
  char region[31];  // "*" = pas de restriction régionale
};

struct EnergySettings {
  uint32_t poll_interval_ms;     // intervalle lecture capteurs
  uint32_t mppt_wdt_interval_ms; // intervalle feed watchdog MPPT
  bool mppt_wdt_enabled;
  bool low_voltage_cutoff_enabled; // active la supervision cutoff_enabled des relais (cf. Settings::Relay)

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
  bool telemetry_log_enabled;          // active l'enregistrement périodique de télémétrie en EEPROM (cf. TelemetryHistory)
  bool event_log_enabled;              // active la journalisation des événements critiques en EEPROM (cf. EventLogHistory)
  uint8_t log_level;                   // LogLevel (0=none .. 5=trace)
  uint8_t mode;                        // OperatingMode (0..4, cf. ci-dessus)
};

#define RELAY_COUNT       4

struct Settings {
  uint32_t magic;
  uint16_t version;

  AprsSettings aprs;
  RadioSettings radio;
  WeatherSettings weather;
  EnergySettings energy;
  SystemSettings system;
  CwSstvSettings cw_sstv;
  MeshChannelSettings mesh_channels[MAX_GROUP_CHANNELS];

  // Relais bistable individuel : état ON/OFF persisté + ses deux règles de
  // supervision automatique — cf. energy/Relay.h pour la logique qui les
  // consomme (une instance construite avec un pointeur direct sur son
  // Settings::Relay, cf. task_energy.cpp). Regroupées ici plutôt qu'en 3
  // tableaux parallèles indexés par relais : un relais et ses règles ne font
  // sens que pris ensemble.
  struct Relay {
    bool state;   // état ON/OFF courant (persisté, cf. hal/relay/RelayHal.h)

    // Coupure basse-tension avec hystérésis réelle : si la tension batterie
    // (mini de battery_mppt/battery_ina) descend sous min_voltage_mv, le
    // relais est coupé (cf. energy/Relay.cpp) ; il est reconnecté
    // automatiquement une fois la tension remontée au-dessus de
    // restore_voltage_mv (doit être > min_voltage_mv). restore_voltage_mv = 0
    // désactive la reconnexion automatique (coupure seule, reconnexion
    // manuelle via CLI "relay.N.state").
    // debounce_ms : la tension doit rester en continu au-delà du seuil
    // (coupure ou reprise) pendant cette durée avant que l'action ne soit
    // prise — évite qu'une chute de tension transitoire (appel de courant
    // bref) ne déclenche une coupure inutile. 0 = action immédiate (pas de
    // confirmation).
    bool cutoff_enabled;
    uint16_t min_voltage_mv;
    uint16_t restore_voltage_mv;
    uint32_t debounce_ms;

    // Réveil périodique : indépendamment de la coupure basse-tension, allume
    // le relais pendant on_duration_ms toutes les interval_ms.
    // override_low_voltage décide si ce réveil a lieu même si la coupure
    // basse-tension maintiendrait le relais éteint (true), ou s'il est
    // simplement sauté ce cycle-là (false).
    bool periodic_enabled;
    uint32_t interval_ms;
    uint32_t on_duration_ms;
    bool override_low_voltage;
  };
  Relay relay[RELAY_COUNT];
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
  s.radio.aprs_cad_enabled = false;

  // Weather
  s.weather.wh65b_enabled = true;
  s.weather.wh65b_interval_ms = 600000;    // 10min
  s.weather.wh65b_rx_timeout_ms = 60000;   // 60s
  s.weather.resend_enabled = false;
  s.weather.resend_power_dbm = 10;
  s.weather.resend_delay_ms = 5000;        // 5s

  // CW + SSTV (radio 433, identification + envoi d'image)
  s.cw_sstv.sstv_mode = 0;                 // cf. SSTV_MODE_TABLE[0] (aprs/SstvTransmitter.h)
  s.cw_sstv.freq_mhz = 437.000f;
  s.cw_sstv.cw_wpm = 20;
  s.cw_sstv.cw_repeats = 3;
  s.cw_sstv.power_dbm = 22;

  // Canaux MeshCore — canal 0 = "Public" (PSK bien connue) par défaut, les
  // autres slots inutilisés (name[0] == '\0')
  for (auto& ch : s.mesh_channels) {
    ch.name[0] = '\0';
    ch.region[0] = '\0';
  }
  strncpy(s.mesh_channels[0].name, "Public", sizeof(s.mesh_channels[0].name));
  strncpy(s.mesh_channels[0].region, "*", sizeof(s.mesh_channels[0].region));

  // Energy
  s.energy.poll_interval_ms = 30000;       // 30s
  s.energy.mppt_wdt_interval_ms = 90000;   // 90s
  s.energy.mppt_wdt_enabled = true;
  s.energy.low_voltage_cutoff_enabled = false;
  s.energy.mppt_pwr_off_mv = 11500;
  s.energy.mppt_pwr_on_mv = 12500;

  // System
  strncpy(s.system.admin_password, "hvv", sizeof(s.system.admin_password));
  s.system.watchdog_enabled = false;
  s.system.telemetry_log_interval_ms = 300000; // 5min
  s.system.telemetry_log_enabled = true;
  s.system.event_log_enabled = true;
  s.system.log_level = 3;                      // INFO par défaut
  s.system.mode = MODE_FULL;                   // fonctionnement normal par défaut

  // Relais — état et règles toutes désactivées par défaut
  for (auto& relay : s.relay) {
    relay.state = false;

    relay.cutoff_enabled = false;
    relay.min_voltage_mv = 0;
    relay.restore_voltage_mv = 0;
    relay.debounce_ms = 60000;

    relay.periodic_enabled = false;
    relay.interval_ms = 600000;   // 10 min (inerte tant que periodic_enabled=false)
    relay.on_duration_ms = 60000; // 1 min
    relay.override_low_voltage = false;
  }

  return s;
}

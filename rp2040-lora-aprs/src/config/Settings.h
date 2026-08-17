#pragma once

#include <stdint.h>
#include <cstring>

#define SETTINGS_MAGIC    0x34485656  // "4HVV"
#define SETTINGS_VERSION  10  // v10 : + EnergySettings::relay_manual_timeout_ms

// Modifiable à chaud ("set system.mode N") mais ne prend effet qu'au reboot (tâches créées une seule fois dans setup()).
enum OperatingMode : uint8_t {
  MODE_APRS_ONLY      = 0,
  MODE_MESHCORE_ONLY  = 1,
  MODE_APRS_MESHCORE  = 2,
  MODE_FULL           = 3,
  MODE_TELEMETRY_ONLY = 4,
};

inline bool modeHasAprs(uint8_t mode) {
  return mode == MODE_APRS_ONLY || mode == MODE_APRS_MESHCORE || mode == MODE_FULL;
}
inline bool modeHasMeshcore(uint8_t mode) {
  return mode == MODE_MESHCORE_ONLY || mode == MODE_APRS_MESHCORE || mode == MODE_FULL;
}
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
  char callsign[10];
  char destination[7];
  char path[16];
  char pathTelemetry[16];
  char symbol;
  char symbolTable;
  float latitude;
  float longitude;
  int16_t altitude;
  bool digipeaterEnabled;
  uint32_t intervalPosition_ms;
  uint32_t intervalTelemetry_ms;
  uint32_t intervalWeather_ms;
  uint32_t intervalStatus_ms;  // filet de sécurité : envoi forcé même si l'état n'a pas changé
  char comment[32];
};

struct RadioSettings {
  float aprs_freq;
  float aprs_bw;
  uint8_t aprs_sf;
  uint8_t aprs_cr;
  int8_t aprs_tx_power;
  bool aprs_cad_enabled;
};

struct WeatherSettings {
  bool wh65b_enabled;
  uint32_t wh65b_interval_ms;
  uint32_t wh65b_rx_timeout_ms;

  // Retransmission RF brute d'une trame WH65B décodée (mêmes octets, même fréquence) : pas une conversion APRS.
  bool resend_enabled;
  int8_t resend_power_dbm;
  uint32_t resend_delay_ms;
};

struct CwSstvSettings {
  uint8_t sstv_mode;
  float freq_mhz;
  uint8_t cw_wpm;
  uint8_t cw_repeats;
  int8_t power_dbm;
};

// Seuls nom et région sont persistés : le secret du canal est dérivé du nom par sha256 au chargement.
struct MeshChannelSettings {
  char name[32];    // "" = slot inutilisé
  char region[31];
};

struct EnergySettings {
  uint32_t poll_interval_ms;
  uint32_t mppt_wdt_interval_ms;
  bool mppt_wdt_enabled;

  // 0 = ne pas toucher au réglage usine de la carte MPPT (poussé au chip seulement si > 0).
  uint16_t mppt_pwr_off_mv;
  uint16_t mppt_pwr_on_mv;

  // Au bout de ce délai en mode manuel, le relais rend la main à cutoff/périodique. 0 = jamais.
  uint32_t relay_manual_timeout_ms;
};

struct SystemSettings {
  char admin_password[16];
  bool watchdog_enabled;
  uint32_t telemetry_log_interval_ms;
  bool telemetry_log_enabled;
  bool event_log_enabled;
  uint8_t log_level;
  uint8_t mode;
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

  // L'état ON/OFF courant n'est pas ici (non persisté) : il vit dans RelayHal, restauré seulement
  // par un reset à chaud ; une vraie coupure secteur repart à "éteint" sans conséquence sur le relais bistable.
  struct Relay {
    // debounce_ms : la tension doit rester du côté opposé à la décision courante pendant ce délai avant bascule.
    bool cutoff_enabled;
    uint16_t min_voltage_mv;
    uint16_t restore_voltage_mv;
    uint32_t debounce_ms;

    bool periodic_enabled;
    uint32_t interval_ms;
    uint32_t on_duration_ms;

    // false (déf.) : cutoff fait garde-fou, ON seulement si les deux règles sont d'accord.
    // true : periodic gouverne seul, jamais bridé par la tension.
    bool override_low_voltage;
  };
  Relay relay[RELAY_COUNT];
};

inline Settings getDefaultSettings() {
  Settings s = {};

  s.magic = SETTINGS_MAGIC;
  s.version = SETTINGS_VERSION;

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
  s.aprs.intervalPosition_ms = 43200000;
  s.aprs.intervalTelemetry_ms = 3600000;
  s.aprs.intervalWeather_ms = 900000;
  s.aprs.intervalStatus_ms = 21600000;
  strncpy(s.aprs.comment, "APRS Digi + Meshcore", sizeof(s.aprs.comment));

  s.radio.aprs_freq = 433.775f;
  s.radio.aprs_bw = 125.0f;
  s.radio.aprs_sf = 12;
  s.radio.aprs_cr = 5;
  s.radio.aprs_tx_power = 22;
  s.radio.aprs_cad_enabled = false;

  s.weather.wh65b_enabled = true;
  s.weather.wh65b_interval_ms = 600000;
  s.weather.wh65b_rx_timeout_ms = 60000;
  s.weather.resend_enabled = false;
  s.weather.resend_power_dbm = 10;
  s.weather.resend_delay_ms = 5000;

  s.cw_sstv.sstv_mode = 0;
  s.cw_sstv.freq_mhz = 437.000f;
  s.cw_sstv.cw_wpm = 20;
  s.cw_sstv.cw_repeats = 3;
  s.cw_sstv.power_dbm = 22;

  // Canal 0 = "Public" (PSK bien connue) par défaut, autres slots inutilisés.
  for (auto& ch : s.mesh_channels) {
    ch.name[0] = '\0';
    ch.region[0] = '\0';
  }
  strncpy(s.mesh_channels[0].name, "Public", sizeof(s.mesh_channels[0].name));
  strncpy(s.mesh_channels[0].region, "*", sizeof(s.mesh_channels[0].region));

  s.energy.poll_interval_ms = 30000;
  s.energy.mppt_wdt_interval_ms = 90000;
  s.energy.mppt_wdt_enabled = true;
  s.energy.mppt_pwr_off_mv = 11500;
  s.energy.mppt_pwr_on_mv = 12500;
  s.energy.relay_manual_timeout_ms = 86400000;

  strncpy(s.system.admin_password, "hvv", sizeof(s.system.admin_password));
  s.system.watchdog_enabled = false;
  s.system.telemetry_log_interval_ms = 300000;
  s.system.telemetry_log_enabled = true;
  s.system.event_log_enabled = true;
  s.system.log_level = 3;
  s.system.mode = MODE_FULL;

  for (auto& relay : s.relay) {
    relay.cutoff_enabled = false;
    relay.min_voltage_mv = 0;
    relay.restore_voltage_mv = 30000;
    relay.debounce_ms = 60000;

    relay.periodic_enabled = false;
    relay.interval_ms = 600000;
    relay.on_duration_ms = 60000;
    relay.override_low_voltage = false;
  }

  return s;
}

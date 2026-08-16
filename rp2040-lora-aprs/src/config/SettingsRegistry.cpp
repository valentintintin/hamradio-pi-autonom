#include "SettingsRegistry.h"
#include "core/Log.h"
#include "aprs/SstvTransmitter.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// Table nom<->valeur pour system.log_level, dérivée des helpers existants de core/Log.h
static const EnumNameEntry LOG_LEVEL_NAMES[] = {
  { "none",  LOG_NONE },
  { "error", LOG_ERROR },
  { "warn",  LOG_WARN },
  { "info",  LOG_INFO },
  { "debug", LOG_DEBUG },
  { "trace", LOG_TRACE },
};

// ============================================================================
// Initialise la table avec tous les champs configurables
// ============================================================================
void SettingsRegistry::init(Settings& s) {
  _count = 0;

  // --- APRS ----------------------------------------------------------------
  add("aprs.callsign",       ST_STRING, s.aprs.callsign,       sizeof(s.aprs.callsign));
  add("aprs.destination",    ST_STRING, s.aprs.destination,     sizeof(s.aprs.destination));
  add("aprs.path",           ST_STRING, s.aprs.path,            sizeof(s.aprs.path));
  add("aprs.pathTelemetry",  ST_STRING, s.aprs.pathTelemetry,   sizeof(s.aprs.pathTelemetry));
  add("aprs.symbol",         ST_INT8,   &s.aprs.symbol);
  add("aprs.symbolTable",    ST_INT8,   &s.aprs.symbolTable);
  add("aprs.latitude",       ST_FLOAT,  &s.aprs.latitude,       -90.0f, 90.0f);
  add("aprs.longitude",      ST_FLOAT,  &s.aprs.longitude,      -180.0f, 180.0f);
  add("aprs.altitude",       ST_INT16,  &s.aprs.altitude,       -500.0f, 9999.0f);
  add("aprs.digipeat",       ST_BOOL,   &s.aprs.digipeaterEnabled);
  add("aprs.interval.position",  ST_UINT32, &s.aprs.intervalPosition_ms,  60000.0f, 86400000.0f);
  add("aprs.interval.telemetry", ST_UINT32, &s.aprs.intervalTelemetry_ms, 60000.0f, 86400000.0f);
  add("aprs.interval.weather",   ST_UINT32, &s.aprs.intervalWeather_ms,   60000.0f, 86400000.0f);
  add("aprs.interval.status",    ST_UINT32, &s.aprs.intervalStatus_ms,    60000.0f, 86400000.0f);
  add("aprs.comment",        ST_STRING, s.aprs.comment,         sizeof(s.aprs.comment));

  // --- Radio ---------------------------------------------------------------
  add("radio.aprs.freq",     ST_FLOAT,  &s.radio.aprs_freq,     430.0f, 440.0f);
  add("radio.aprs.bw",       ST_FLOAT,  &s.radio.aprs_bw,       7.8f, 500.0f);
  add("radio.aprs.sf",       ST_UINT8,  &s.radio.aprs_sf,       5.0f, 12.0f);
  add("radio.aprs.cr",       ST_UINT8,  &s.radio.aprs_cr,       5.0f, 8.0f);
  add("radio.aprs.power",    ST_INT8,   &s.radio.aprs_tx_power, -9.0f, 22.0f);
  add("radio.aprs.cad",      ST_BOOL,   &s.radio.aprs_cad_enabled);

  // --- Weather -------------------------------------------------------------
  add("weather.wh65b.enabled",  ST_BOOL,   &s.weather.wh65b_enabled);
  add("weather.wh65b.interval", ST_UINT32, &s.weather.wh65b_interval_ms,  60000.0f, 3600000.0f);
  add("weather.wh65b.timeout",  ST_UINT32, &s.weather.wh65b_rx_timeout_ms, 5000.0f, 120000.0f);
  add("weather.resend.enabled", ST_BOOL,   &s.weather.resend_enabled);
  add("weather.resend_power_dbm", ST_INT8, &s.weather.resend_power_dbm, -9.0f, 22.0f);
  add("weather.resend_delay_ms",  ST_UINT32, &s.weather.resend_delay_ms, 0.0f, 60000.0f);

  // --- CW (identification morse) + SSTV (envoi d'image), radio 433 --------
  add("sstv.mode",     &s.cw_sstv.sstv_mode, SSTV_MODE_NAMES, SSTV_MODE_COUNT);
  add("sstv.freq_mhz", ST_FLOAT, &s.cw_sstv.freq_mhz,   400.0f, 470.0f);
  add("sstv.cw_wpm",   ST_UINT8, &s.cw_sstv.cw_wpm,      5.0f, 40.0f);
  add("sstv.cw_repeats", ST_UINT8, &s.cw_sstv.cw_repeats, 1.0f, 10.0f);
  add("sstv.power_dbm", ST_INT8, &s.cw_sstv.power_dbm,  -9.0f, 22.0f);

  // --- Canaux de groupe MeshCore (mesh.channel.N.name/region) --------------
  // Clés générées dynamiquement (nombre de canaux = MAX_GROUP_CHANNELS,
  // configurable au build) : buffers `static` car add() garde le pointeur de
  // la clé tel quel (pas de copie), donc une variable locale non-static ici
  // laisserait un pointeur pendouillant après le retour de la boucle.
  static char channel_keys[MAX_GROUP_CHANNELS][2][24];
  for (int i = 0; i < MAX_GROUP_CHANNELS; i++) {
    snprintf(channel_keys[i][0], sizeof(channel_keys[i][0]), "mesh.channel.%d.name", i);
    snprintf(channel_keys[i][1], sizeof(channel_keys[i][1]), "mesh.channel.%d.region", i);
    add(channel_keys[i][0], ST_STRING, s.mesh_channels[i].name,   sizeof(s.mesh_channels[i].name));
    add(channel_keys[i][1], ST_STRING, s.mesh_channels[i].region, sizeof(s.mesh_channels[i].region));
  }

  // --- Energy --------------------------------------------------------------
  add("energy.poll_interval",     ST_UINT32, &s.energy.poll_interval_ms,     5000.0f, 600000.0f);
  add("energy.mppt_wdt.interval", ST_UINT32, &s.energy.mppt_wdt_interval_ms, 10000.0f, 300000.0f);
  add("energy.mppt_wdt.enabled",  ST_BOOL,   &s.energy.mppt_wdt_enabled);
  add("energy.low_voltage_cutoff.enabled", ST_BOOL, &s.energy.low_voltage_cutoff_enabled);
  add("energy.mppt_pwr_off_mv", ST_UINT16, &s.energy.mppt_pwr_off_mv, 0.0f, 30000.0f);
  add("energy.mppt_pwr_on_mv",  ST_UINT16, &s.energy.mppt_pwr_on_mv,  0.0f, 30000.0f);

  // --- System --------------------------------------------------------------
  add("system.password",       ST_STRING, s.system.admin_password, sizeof(s.system.admin_password));
  add("system.watchdog",       ST_BOOL,   &s.system.watchdog_enabled);
  add("system.telemetry_log_interval",   ST_UINT32, &s.system.telemetry_log_interval_ms, 10000.0f, 3600000.0f);
  add("system.telemetry_log.enabled", ST_BOOL, &s.system.telemetry_log_enabled);
  add("system.event_log.enabled",     ST_BOOL, &s.system.event_log_enabled);
  add("system.log_level",      &s.system.log_level, LOG_LEVEL_NAMES, sizeof(LOG_LEVEL_NAMES) / sizeof(LOG_LEVEL_NAMES[0]));
  // OperatingMode : 0=aprs, 1=meshcore, 2=aprs+meshcore, 3=full, 4=telemetry (cf. config/Settings.h)
  add("system.mode",           ST_UINT8,  &s.system.mode,      0.0f, 4.0f);

  // --- Relais (bistables, pilotés via TCA9555 I2C — cf. hal/relay/RelayHal.h) ----
  add("relay.1.state", ST_BOOL, &s.relay[0].state);
  add("relay.2.state", ST_BOOL, &s.relay[1].state);
  add("relay.3.state", ST_BOOL, &s.relay[2].state);
  add("relay.4.state", ST_BOOL, &s.relay[3].state);

  // --- Coupure basse-tension (cf. energy/Relay.cpp) -------------------------
  // Clés CLI inchangées ("relay_cutoff.N.*") bien que la donnée soit
  // désormais dans s.relay[N] (cf. Settings::Relay) — pas de raison de casser
  // la compatibilité CLI/scripts pour un détail de layout interne.
  add("relay_cutoff.1.enabled",       ST_BOOL,  &s.relay[0].cutoff_enabled);
  add("relay_cutoff.1.min_mv",      ST_UINT16, &s.relay[0].min_voltage_mv,     0.0f, 30000.0f);
  add("relay_cutoff.1.restore_mv",  ST_UINT16, &s.relay[0].restore_voltage_mv, 0.0f, 30000.0f);
  add("relay_cutoff.1.debounce_ms", ST_UINT32, &s.relay[0].debounce_ms,        0.0f, 600000.0f);
  add("relay_cutoff.2.enabled",       ST_BOOL,  &s.relay[1].cutoff_enabled);
  add("relay_cutoff.2.min_mv",      ST_UINT16, &s.relay[1].min_voltage_mv,     0.0f, 30000.0f);
  add("relay_cutoff.2.restore_mv",  ST_UINT16, &s.relay[1].restore_voltage_mv, 0.0f, 30000.0f);
  add("relay_cutoff.2.debounce_ms", ST_UINT32, &s.relay[1].debounce_ms,        0.0f, 600000.0f);
  add("relay_cutoff.3.enabled",       ST_BOOL,  &s.relay[2].cutoff_enabled);
  add("relay_cutoff.3.min_mv",      ST_UINT16, &s.relay[2].min_voltage_mv,     0.0f, 30000.0f);
  add("relay_cutoff.3.restore_mv",  ST_UINT16, &s.relay[2].restore_voltage_mv, 0.0f, 30000.0f);
  add("relay_cutoff.3.debounce_ms", ST_UINT32, &s.relay[2].debounce_ms,        0.0f, 600000.0f);
  add("relay_cutoff.4.enabled",       ST_BOOL,  &s.relay[3].cutoff_enabled);
  add("relay_cutoff.4.min_mv",      ST_UINT16, &s.relay[3].min_voltage_mv,     0.0f, 30000.0f);
  add("relay_cutoff.4.restore_mv",  ST_UINT16, &s.relay[3].restore_voltage_mv, 0.0f, 30000.0f);
  add("relay_cutoff.4.debounce_ms", ST_UINT32, &s.relay[3].debounce_ms,        0.0f, 600000.0f);

  // --- Réveil périodique par relais (cf. energy/Relay.cpp) ------------------
  add("relay_periodic.1.enabled",      ST_BOOL,   &s.relay[0].periodic_enabled);
  add("relay_periodic.1.interval_ms",  ST_UINT32, &s.relay[0].interval_ms,     60000.0f, 86400000.0f);
  add("relay_periodic.1.on_duration_ms", ST_UINT32, &s.relay[0].on_duration_ms, 1000.0f, 3600000.0f);
  add("relay_periodic.1.override_low_voltage", ST_BOOL, &s.relay[0].override_low_voltage);
  add("relay_periodic.2.enabled",      ST_BOOL,   &s.relay[1].periodic_enabled);
  add("relay_periodic.2.interval_ms",  ST_UINT32, &s.relay[1].interval_ms,     60000.0f, 86400000.0f);
  add("relay_periodic.2.on_duration_ms", ST_UINT32, &s.relay[1].on_duration_ms, 1000.0f, 3600000.0f);
  add("relay_periodic.2.override_low_voltage", ST_BOOL, &s.relay[1].override_low_voltage);
  add("relay_periodic.3.enabled",      ST_BOOL,   &s.relay[2].periodic_enabled);
  add("relay_periodic.3.interval_ms",  ST_UINT32, &s.relay[2].interval_ms,     60000.0f, 86400000.0f);
  add("relay_periodic.3.on_duration_ms", ST_UINT32, &s.relay[2].on_duration_ms, 1000.0f, 3600000.0f);
  add("relay_periodic.3.override_low_voltage", ST_BOOL, &s.relay[2].override_low_voltage);
  add("relay_periodic.4.enabled",      ST_BOOL,   &s.relay[3].periodic_enabled);
  add("relay_periodic.4.interval_ms",  ST_UINT32, &s.relay[3].interval_ms,     60000.0f, 86400000.0f);
  add("relay_periodic.4.on_duration_ms", ST_UINT32, &s.relay[3].on_duration_ms, 1000.0f, 3600000.0f);
  add("relay_periodic.4.override_low_voltage", ST_BOOL, &s.relay[3].override_low_voltage);
}

// ============================================================================
// Ajouter une entrée (sans bornes)
// ============================================================================
void SettingsRegistry::add(const char* key, SettingType type, void* ptr, uint8_t maxLen) {
  if (_count >= 128) {
    return;
  }
  _entries[_count++] = { key, type, ptr, maxLen, 0, 0, false, nullptr, 0 };
}

// ============================================================================
// Ajouter une entrée (avec bornes min/max)
// ============================================================================
void SettingsRegistry::add(const char* key, SettingType type, void* ptr, float min, float max) {
  if (_count >= 128) {
    return;
  }
  _entries[_count++] = { key, type, ptr, 0, min, max, true, nullptr, 0 };
}

// ============================================================================
// Ajouter une entrée enum par nom (ST_ENUM8, cf. SettingsRegistry.h)
// ============================================================================
void SettingsRegistry::add(const char* key, void* ptr, const EnumNameEntry* table, uint8_t tableCount) {
  if (_count >= 128) {
    return;
  }
  _entries[_count++] = { key, ST_ENUM8, ptr, 0, 0, 0, false, table, tableCount };
}

// ============================================================================
// Validation des bornes
// ============================================================================
bool SettingsRegistry::validate(const SettingEntry* e, float value, Print* out) const {
  if (!e->hasRange) {
    return true;
  }
  if (value < e->min || value > e->max) {
    if (out) {
      out->printf("Hors limites: %.4g (attendu [%.4g, %.4g])\n", value, e->min, e->max);
    }
    return false;
  }
  return true;
}

// ============================================================================
// Chercher par clé
// ============================================================================
const SettingEntry* SettingsRegistry::find(const char* key) const {
  for (int i = 0; i < _count; i++) {
    if (strcmp(_entries[i].key, key) == 0) {
      return &_entries[i];
    }
  }
  return nullptr;
}

// ============================================================================
// Lire en string
// ============================================================================
bool SettingsRegistry::get(const char* key, char* out, size_t outLen) const {
  const SettingEntry* e = find(key);
  if (!e) {
    return false;
  }

  switch (e->type) {
    case ST_STRING:
      strncpy(out, (const char*)e->ptr, outLen - 1);
      out[outLen - 1] = '\0';
      break;
    case ST_FLOAT:
      snprintf(out, outLen, "%.4f", *(float*)e->ptr);
      break;
    case ST_INT8:
      snprintf(out, outLen, "%d", (int)*(int8_t*)e->ptr);
      break;
    case ST_INT16:
      snprintf(out, outLen, "%d", (int)*(int16_t*)e->ptr);
      break;
    case ST_UINT8:
      snprintf(out, outLen, "%u", (unsigned)*(uint8_t*)e->ptr);
      break;
    case ST_UINT16:
      snprintf(out, outLen, "%u", (unsigned)*(uint16_t*)e->ptr);
      break;
    case ST_UINT32:
      snprintf(out, outLen, "%lu", *(uint32_t*)e->ptr);
      break;
    case ST_BOOL:
      strncpy(out, *(bool*)e->ptr ? "true" : "false", outLen);
      break;
    case ST_ENUM8: {
      uint8_t v = *(uint8_t*)e->ptr;
      const char* name = "?";
      for (uint8_t i = 0; i < e->enumTableCount; i++) {
        if (e->enumTable[i].value == v) {
          name = e->enumTable[i].name;
          break;
        }
      }
      strncpy(out, name, outLen - 1);
      out[outLen - 1] = '\0';
      break;
    }
  }
  return true;
}

// ============================================================================
// Écrire depuis un string (avec validation des bornes)
// ============================================================================
bool SettingsRegistry::set(const char* key, const char* value) {
  return set(key, value, nullptr);
}

bool SettingsRegistry::set(const char* key, const char* value, Print* out) {
  const SettingEntry* e = find(key);
  if (!e) {
    return false;
  }

  switch (e->type) {
    case ST_STRING:
      strncpy((char*)e->ptr, value, e->maxLen - 1);
      ((char*)e->ptr)[e->maxLen - 1] = '\0';
      break;
    case ST_FLOAT: {
      float fv = atof(value);
      if (!validate(e, fv, out)) {
        return false;
      }
      *(float*)e->ptr = fv;
      break;
    }
    case ST_INT8: {
      int iv = atoi(value);
      if (!validate(e, (float)iv, out)) {
        return false;
      }
      *(int8_t*)e->ptr = (int8_t)iv;
      break;
    }
    case ST_INT16: {
      int iv = atoi(value);
      if (!validate(e, (float)iv, out)) {
        return false;
      }
      *(int16_t*)e->ptr = (int16_t)iv;
      break;
    }
    case ST_UINT8: {
      int iv = atoi(value);
      if (!validate(e, (float)iv, out)) {
        return false;
      }
      *(uint8_t*)e->ptr = (uint8_t)iv;
      break;
    }
    case ST_UINT16: {
      int iv = atoi(value);
      if (!validate(e, (float)iv, out)) {
        return false;
      }
      *(uint16_t*)e->ptr = (uint16_t)iv;
      break;
    }
    case ST_UINT32: {
      uint32_t uv = (uint32_t)strtoul(value, nullptr, 10);
      if (!validate(e, (float)uv, out)) {
        return false;
      }
      *(uint32_t*)e->ptr = uv;
      break;
    }
    case ST_BOOL:
      *(bool*)e->ptr = (strcmp(value, "true") == 0 || strcmp(value, "1") == 0 || strcmp(value, "on") == 0);
      break;
    case ST_ENUM8: {
      for (uint8_t i = 0; i < e->enumTableCount; i++) {
        if (strcmp(e->enumTable[i].name, value) == 0) {
          *(uint8_t*)e->ptr = e->enumTable[i].value;
          return true;
        }
      }
      if (out) {
        out->printf("Valeur invalide '%s', attendu: ", value);
        for (uint8_t i = 0; i < e->enumTableCount; i++) {
          out->printf("%s%s", i == 0 ? "" : "/", e->enumTable[i].name);
        }
        out->printf("\n");
      }
      return false;
    }
  }
  return true;
}

// ============================================================================
// Lister toutes les entrées (avec bornes si définies)
// ============================================================================
void SettingsRegistry::listAll(Print& out) const {
  char buf[64];
  for (int i = 0; i < _count; i++) {
    get(_entries[i].key, buf, sizeof(buf));
    if (_entries[i].hasRange) {
      out.printf("  %-30s = %-16s [%.4g..%.4g]\n",
        _entries[i].key, buf, _entries[i].min, _entries[i].max);
    } else {
      out.printf("  %-30s = %s\n", _entries[i].key, buf);
    }
  }
}

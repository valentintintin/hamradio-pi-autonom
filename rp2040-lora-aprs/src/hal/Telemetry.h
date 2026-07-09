#pragma once

#include <stdint.h>

// ============================================================================
// Struct de télémétrie centralisée
// Remplie par les HAL capteurs, lue par les tasks (beacon, bridge, CLI)
// ============================================================================

struct EnergyData {
  float voltage_mv;     // tension en mV
  float current_ma;     // courant en mA
};

struct WeatherData {
  float temperature_c;
  float humidity;       // 0-100 %
  float pressure_hpa;
  // WH65B (station extérieure)
  bool  wh65b_valid;
  float wind_avg_ms;
  float wind_max_ms;
  int   wind_dir_deg;
  float rain_mm;
  float light_lux;
  int   uv_index;
};

struct Telemetry {
  // INA3221 — 3 canaux
  EnergyData battery;
  EnergyData solar;
  EnergyData board;

  // MPPT charger
  float mppt_charge_ma;
  float mppt_int_temp_c;
  uint16_t mppt_status;
  bool  mppt_alert;
  bool  mppt_night;

  // BME280
  WeatherData weather;

  // Victron VE.Direct
  float victron_voltage_mv;
  float victron_current_ma;
  float victron_power_w;
  float victron_soc;          // State of charge (%)

  // Uptime
  unsigned long uptime_s;

  // Timestamp dernière mise à jour
  unsigned long last_update_ms;
};

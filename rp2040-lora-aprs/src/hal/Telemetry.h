#pragma once

#include <stdint.h>

struct EnergyData {
  float voltage_mv;
  float current_ma;
};

struct WeatherData {
  float temperature_c;
  float humidity;       // 0-100 %
};

// Anciennement des `union` : bug, tous les champs sont écrits/lus simultanément
// (pas les uns à la place des autres) donc chaque écriture corrompait les précédentes.
struct WeatherDataBme {
  WeatherData base;
  float pressure_hpa;
};

struct WeatherDataExtended {
  WeatherData base;
  bool is_valid;
  float wind_avg_ms;
  float wind_max_ms;
  int   wind_dir_deg;
  float rain_mm;
  float light_lux;
  int   uv_index;
};

struct TelemetryData {
  EnergyData battery_ina;
  EnergyData solar_ina;
  EnergyData board_5v;

  EnergyData battery_mppt;
  EnergyData solar_mppt;

  uint16_t mppt_status;

  WeatherDataBme weather_inside;
  WeatherDataExtended weather_outside;

  float victron_soc; // % (Victron VE.Direct)

  uint32_t uptime_s;

  uint32_t last_update_ms;
};

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

union WeatherDataBme {
  WeatherData base;
  float pressure_hpa;
};

union WeatherDataExtended {
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
  // INA3221 — 3 canaux
  EnergyData battery_ina;
  EnergyData solar_ina;
  EnergyData board_5v;

  EnergyData battery_mppt;
  EnergyData solar_mppt;

  // MPPT charger
  uint16_t mppt_status;

  // BME280
  WeatherDataBme weather_inside;
  WeatherDataExtended weather_outside;

  // Victron VE.Direct
  float victron_soc;          // State of charge (%)

  uint32_t uptime_s;

  uint32_t last_update_ms;
};

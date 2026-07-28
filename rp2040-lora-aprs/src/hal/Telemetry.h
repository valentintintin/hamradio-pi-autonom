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

// Anciennement des `union` — bug : tous les champs sont en réalité écrits et
// lus simultanément (cf. hal/sensors/Bme280Hal.h::query(), tasks/
// task_weather.cpp, aprs/AprsEventHandler.cpp), pas les uns à la place des
// autres. En union, chaque écriture écrasait les précédentes (même
// stockage) — température intérieure et tous les champs météo extérieurs
// (vent, pluie, direction, validité) étaient corrompus dès la 2e écriture.
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

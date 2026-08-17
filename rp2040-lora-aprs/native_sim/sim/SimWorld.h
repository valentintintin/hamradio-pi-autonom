#pragma once

#include <cstdint>
#include <cstddef>
#include <mutex>
#include <deque>
#include <string>
#include <vector>

struct RadioLogEntry {
  bool tx;
  uint32_t millis_ts;
  std::vector<uint8_t> data;
  float rssi = 0, snr = 0;
};

struct RxQueueEntry {
  std::vector<uint8_t> data;
  float rssi = -90.0f;
  float snr = 8.0f;
};

class SimWorld {
public:
  static SimWorld& instance();

  std::mutex mutex;

  float temperature_c = 21.5f;
  float humidity_pct = 48.0f;
  float pressure_hpa = 1013.0f;

  float battery_mv = 12800.0f, battery_ma = -180.0f;
  float board5v_mv = 5000.0f, board5v_ma = 120.0f;
  float solar_ina_mv = 18000.0f, solar_ina_ma = 350.0f;

  bool mppt_present = true;
  float mppt_battery_mv = 12800.0f, mppt_battery_ma = -180.0f;
  float mppt_solar_mv = 18000.0f, mppt_solar_ma = 350.0f;
  bool mppt_alert = false;
  uint16_t mppt_charge_state = 0;  // MPPT_CHG_ST_* (lib/mpptChg/mpptChg.h), 0 = NIGHT

  bool victron_present = false;
  float victron_soc_pct = 85.0f;
  int victron_state = 3;  // 3 = Bulk (VE.Direct "state of operation")

  bool tca9555_present = true;
  uint16_t tca9555_output = 0x0000;
  uint16_t tca9555_config = 0xFFFF;  // 1 = entrée (valeur usine), 0 = sortie

  static constexpr size_t kRadioLogMax = 40;
  std::deque<RadioLogEntry> mesh_log, aprs_log;
  std::deque<RxQueueEntry> mesh_rx_queue, aprs_rx_queue;

  bool fsk_mode = false;
  std::deque<RadioLogEntry> fsk_log;
  std::deque<RxQueueEntry> fsk_rx_queue;

  void logTx(std::deque<RadioLogEntry>& log, const uint8_t* data, int len);
  void pushLog(std::deque<RadioLogEntry>& log, RadioLogEntry entry);

  static constexpr size_t kLogRingMax = 300;
  std::deque<std::string> log_ring;
  void pushLogLine(const std::string& line);
};

#include "SimWebBridge.h"
#include "SimWorld.h"
#include "SimPaths.h"
#include "SimCli.h"
#include "core/Log.h"
#include "core/StringPrint.h"
#include "hal/Telemetry.h"
#include "hal/relay/Relay.h"
#include "cli/CommandHandler.h"
#include "config/Settings.h"
#include "target.h"
#include <mpptChg.h>
#include <ArduinoJson.h>

#include <dirent.h>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <chrono>
#include <string>
#include <thread>
#include <vector>

extern TelemetryData telemetry;
extern CommandHandler command_handler;
extern Settings settings;
extern Relay relays[RELAY_COUNT];

namespace {

std::string webDir() { return simDataDir() + "/web"; }
std::string commandsDir() { return webDir() + "/commands"; }
std::string stateFile() { return webDir() + "/state.json"; }

std::string hexEncode(const std::vector<uint8_t>& data) {
  static const char* kHex = "0123456789ABCDEF";
  std::string out;
  out.reserve(data.size() * 2);
  for (uint8_t b : data) {
    out += kHex[b >> 4];
    out += kHex[b & 0xF];
  }
  return out;
}

void fillEnergy(JsonObject o, const EnergyData& e) {
  o["voltage_mv"] = e.voltage_mv;
  o["current_ma"] = e.current_ma;
}

void fillRadio(JsonObject o, SimRadio& radio, bool withFskMode) {
  o["freq"] = radio.getFreq();
  o["bw"] = radio.getBw();
  o["sf"] = radio.getSf();
  o["cr"] = radio.getCr();
  o["tx_power_dbm"] = radio.getTxPowerDbm();
  o["last_rssi"] = radio.getLastRSSI();
  o["last_snr"] = radio.getLastSNR();
  o["packets_recv"] = radio.getPacketsRecv();
  o["packets_sent"] = radio.getPacketsSent();
  o["cad_busy"] = radio.getCadBusy();
  o["noise_floor_rssi"] = radio.getNoiseFloor();
  if (withFskMode) {
    o["fsk_mode"] = SimWorld::instance().fsk_mode;
  }
}

// ArduinoJson plutôt qu'un ostringstream à la main : sérialise les NaN/Inf
// (météo WH65B invalide) en `null` JSON valide, là où "<<" écrirait "nan"/"inf"
// et ferait planter json.loads() côté serveur Python.
std::string buildStateJson() {
  auto& w = SimWorld::instance();
  JsonDocument doc;
  {
    std::lock_guard<std::mutex> lock(w.mutex);

    JsonObject si = doc["sim_inputs"].to<JsonObject>();
    si["temp_c"] = w.temperature_c;
    si["humidity"] = w.humidity_pct;
    si["pressure_hpa"] = w.pressure_hpa;
    si["battery_mv"] = w.battery_mv;
    si["battery_ma"] = w.battery_ma;
    si["board5v_mv"] = w.board5v_mv;
    si["board5v_ma"] = w.board5v_ma;
    si["solar_mv"] = w.solar_ina_mv;
    si["solar_ma"] = w.solar_ina_ma;
    si["mppt_present"] = w.mppt_present;
    si["mppt_alert"] = w.mppt_alert;
    si["mppt_status_forced"] = mpptChg::getStatusAsString(w.mppt_charge_state);
    si["victron_present"] = w.victron_present;
    si["victron_soc_pct"] = w.victron_soc_pct;
    si["victron_state"] = w.victron_state;

    JsonObject tel = doc["telemetry"].to<JsonObject>();
    fillEnergy(tel["battery_ina"].to<JsonObject>(), telemetry.battery_ina);
    fillEnergy(tel["solar_ina"].to<JsonObject>(), telemetry.solar_ina);
    fillEnergy(tel["board_5v"].to<JsonObject>(), telemetry.board_5v);
    fillEnergy(tel["battery_mppt"].to<JsonObject>(), telemetry.battery_mppt);
    fillEnergy(tel["solar_mppt"].to<JsonObject>(), telemetry.solar_mppt);
    tel["mppt_status"] = telemetry.mppt_status;
    tel["mppt_status_name"] = mpptChg::getStatusAsString(telemetry.mppt_status);
    tel["victron_soc"] = telemetry.victron_soc;
    tel["uptime_s"] = telemetry.uptime_s;

    JsonObject wi = tel["weather_inside"].to<JsonObject>();
    wi["temperature_c"] = telemetry.weather_inside.base.temperature_c;
    wi["humidity"] = telemetry.weather_inside.base.humidity;
    wi["pressure_hpa"] = telemetry.weather_inside.pressure_hpa;

    JsonObject wo = tel["weather_outside"].to<JsonObject>();
    wo["is_valid"] = telemetry.weather_outside.is_valid;
    wo["wind_avg_ms"] = telemetry.weather_outside.wind_avg_ms;
    wo["wind_max_ms"] = telemetry.weather_outside.wind_max_ms;
    wo["wind_dir_deg"] = telemetry.weather_outside.wind_dir_deg;
    wo["rain_mm"] = telemetry.weather_outside.rain_mm;
    wo["light_lux"] = telemetry.weather_outside.light_lux;
    wo["uv_index"] = telemetry.weather_outside.uv_index;

    fillRadio(doc["radio_mesh"].to<JsonObject>(), mesh_radio_driver, false);
    fillRadio(doc["radio_aprs"].to<JsonObject>(), aprs_radio_driver, true);

    JsonObject aprsCfg = doc["aprs_config"].to<JsonObject>();
    aprsCfg["freq"] = settings.radio.aprs_freq;
    aprsCfg["bw"] = settings.radio.aprs_bw;
    aprsCfg["sf"] = settings.radio.aprs_sf;
    aprsCfg["cr"] = settings.radio.aprs_cr;
    aprsCfg["tx_power_dbm"] = settings.radio.aprs_tx_power;

    JsonArray relaysJson = doc["relays"].to<JsonArray>();
    for (int i = 0; i < RELAY_COUNT; i++) {
      JsonObject r = relaysJson.add<JsonObject>();
      r["on"] = relays[i].getState();
      r["manual"] = relays[i].isManualOverride();
    }

    JsonArray radioLog = doc["radio_log"].to<JsonArray>();
    auto dump = [&](std::deque<RadioLogEntry>& log, const char* chan) {
      for (auto& e : log) {
        JsonObject o = radioLog.add<JsonObject>();
        o["ms"] = e.millis_ts;
        o["dir"] = e.tx ? "TX" : "RX";
        o["chan"] = chan;
        o["hex"] = hexEncode(e.data);
      }
    };
    dump(w.mesh_log, "mesh");
    dump(w.aprs_log, "aprs");
    dump(w.fsk_log, "fsk");

    JsonArray logs = doc["logs"].to<JsonArray>();
    for (auto& line : w.log_ring) {
      logs.add(line);
    }
  }

  std::string out;
  serializeJson(doc, out);
  return out;
}

void writeStateFileAtomic() {
  std::string content = buildStateJson();
  std::string path = stateFile();
  std::string tmp = path + ".tmp";
  FILE* f = fopen(tmp.c_str(), "wb");
  if (!f) {
    return;
  }
  fwrite(content.data(), 1, content.size(), f);
  fclose(f);
  ::rename(tmp.c_str(), path.c_str());
}

void exportLoop() {
  simMkdirs(webDir());
  for (;;) {
    writeStateFileAtomic();
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
  }
}

void processCommandFile(const std::string& path) {
  std::string cmd;
  FILE* f = fopen(path.c_str(), "rb");
  if (f) {
    char buf[512];
    size_t n = fread(buf, 1, sizeof(buf) - 1, f);
    buf[n] = '\0';
    cmd = buf;
    fclose(f);
  }
  ::remove(path.c_str());

  while (!cmd.empty() && (cmd.back() == '\n' || cmd.back() == '\r')) {
    cmd.pop_back();
  }
  if (cmd.empty()) {
    return;
  }

  char replyBuf[256];
  StringPrint reply(replyBuf, sizeof(replyBuf));
  if (!simHandleCommand(cmd.c_str(), reply)) {
    command_handler.execute(cmd.c_str(), reply);
  }
  LOG_D("WEB", "commande: %s", cmd.c_str());
}

void commandLoop() {
  std::string dir = commandsDir();
  simMkdirs(dir);
  for (;;) {
    std::vector<std::string> names;
    DIR* d = opendir(dir.c_str());
    if (d) {
      struct dirent* e;
      while ((e = readdir(d)) != nullptr) {
        std::string name = e->d_name;
        if (name.size() > 4 && name.compare(name.size() - 4, 4, ".cmd") == 0) {
          names.push_back(name);
        }
      }
      closedir(d);
    }
    std::sort(names.begin(), names.end());
    for (auto& name : names) {
      processCommandFile(dir + "/" + name);
    }
    if (!names.empty()) {
      // Sans cet appel, server.py (qui attend la disparition du fichier de commande
      // comme signal "fait") répondrait avant que state.json ne reflète l'effet.
      writeStateFileAtomic();
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
}

}  // namespace

void startSimWebBridge() {
  simMkdirs(webDir());
  simMkdirs(commandsDir());
  std::thread(exportLoop).detach();
  std::thread(commandLoop).detach();
  LOG_I("WEB", "Pont web : %s (lancer aussi native_sim/web/server.py)", webDir().c_str());
}

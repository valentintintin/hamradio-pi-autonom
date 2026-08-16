#pragma once

#include <cstdint>
#include <cstddef>
#include <mutex>
#include <deque>
#include <string>
#include <vector>

// ============================================================================
// SimWorld — état simulé partagé de la plupart des périphériques, source
// unique de vérité lue/modifiée à la fois par les HAL simulés (native_sim/
// compat/Adafruit_BME280.h, Adafruit_INA3221.h, TCA9555.h, VEDirect.h, et
// lib/mpptChg en NATIVE_BUILD), par le CLI (native_sim/sim/SimCli.h) et par
// le dashboard web (native_sim/web/WebDashboard.h). Modifier une valeur ici
// la rend immédiatement visible partout.
//
// L'EEPROM (hal/eeprom/M24M01Hal.cpp) fait exception : elle est persistée
// dans de vrais fichiers sous SIM_DATA_DIR/eeprom/ (un par entrée) plutôt
// qu'en mémoire ici — cf. commentaire en tête de ce fichier.
//
// Singleton simple (pas d'injection de dépendance) : les HAL simulés n'ont
// pas de "port" par lequel recevoir une référence (ce sont des remplacements
// de libs vendor construites en dur par le code métier existant), donc un
// point d'accès global est le plus simple ici.
// ============================================================================

struct RadioLogEntry {
  bool tx;  // true = émis, false = reçu (consommé depuis la file d'injection)
  uint32_t millis_ts;
  std::vector<uint8_t> data;
  float rssi = 0, snr = 0;
};

// Entrée d'une file d'injection RX ("sim rx ..."/POST /api/rx) : le RSSI/SNR
// simulés sont désormais portés par la trame elle-même (plutôt qu'une
// constante globale), pour pouvoir tester un paquet à la limite du seuil de
// décodage (SNR faible) sans changer d'état global entre deux injections.
struct RxQueueEntry {
  std::vector<uint8_t> data;
  float rssi = -90.0f;
  float snr = 8.0f;
};

class SimWorld {
public:
  static SimWorld& instance();

  std::mutex mutex;

  // --- Capteurs environnement (BME280) --------------------------------------
  float temperature_c = 21.5f;
  float humidity_pct = 48.0f;
  float pressure_hpa = 1013.0f;

  // --- Alimentation (INA3221 : batterie/board/solaire) ----------------------
  float battery_mv = 12800.0f, battery_ma = -180.0f;
  float board5v_mv = 5000.0f, board5v_ma = 120.0f;
  float solar_ina_mv = 18000.0f, solar_ina_ma = 350.0f;

  // --- Chargeur MPPT (I2C, addr 0x12) ----------------------------------------
  bool mppt_present = true;
  float mppt_battery_mv = 12800.0f, mppt_battery_ma = -180.0f;
  float mppt_solar_mv = 18000.0f, mppt_solar_ma = 350.0f;
  bool mppt_alert = false;
  // État de charge forcé (MPPT_CHG_ST_* dans lib/mpptChg/mpptChg.h, 0=NIGHT
  // par défaut) — lu par mpptChg.cpp (_Read16, branche NATIVE_BUILD) pour
  // composer le registre STATUS, écrit par "sim set mppt status <nom>".
  uint16_t mppt_charge_state = 0;

  // --- Victron VE.Direct (alternative au MPPT selon la révision de carte) ---
  bool victron_present = false;
  // SOC (%) et "state of operation" VE.Direct simulés — lus par
  // native_sim/compat/VEDirect.h, écrits par "sim set victron soc|state".
  float victron_soc_pct = 85.0f;
  int victron_state = 3;  // 3 = Bulk (cf. victron VE.Direct "state of operation")

  // --- Expandeur GPIO TCA9555 (relais, addr 0x20) -----------------------------
  bool tca9555_present = true;
  uint16_t tca9555_output = 0x0000;  // registre de sortie brut (2 ports x 8 bits)
  uint16_t tca9555_config = 0xFFFF;  // 1 = entrée (valeur usine), 0 = sortie

  // --- Radios (mesh 868 / aprs 433) — TX loggé, RX injecté ------------------
  static constexpr size_t kRadioLogMax = 40;
  std::deque<RadioLogEntry> mesh_log, aprs_log;
  std::deque<RxQueueEntry> mesh_rx_queue, aprs_rx_queue;

  // --- FSK météo WH65B (radio APRS basculée en FSK périodiquement, cf.
  // tasks/task_weather.cpp) — file séparée : format WH65B brut (27 octets),
  // pas des trames APRS-sur-LoRa. fsk_mode reflète si la radio est
  // actuellement basculée en écoute FSK (informatif, pour le dashboard).
  bool fsk_mode = false;
  std::deque<RadioLogEntry> fsk_log;
  std::deque<RxQueueEntry> fsk_rx_queue;

  void logTx(std::deque<RadioLogEntry>& log, const uint8_t* data, int len);
  void pushLog(std::deque<RadioLogEntry>& log, RadioLogEntry entry);

  // --- Logs (tee depuis core/Log.h, cf. Boot.cpp) -----------------------------
  static constexpr size_t kLogRingMax = 300;
  std::deque<std::string> log_ring;
  void pushLogLine(const std::string& line);
};

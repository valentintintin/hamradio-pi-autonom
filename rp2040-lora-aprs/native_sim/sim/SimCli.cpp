#include "SimCli.h"
#include "SimWorld.h"
#include "core/RadioActivityNotify.h"
#include <target.h>  // mesh_radio_driver / aprs_radio_driver (SimRadio)

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <strings.h>
#include <vector>
#include <mpptChg.h>

namespace {

// Résout "mesh"/"aprs" vers l'instance SimRadio correspondante (cf.
// variant_native/target.h) — nullptr si le nom ne correspond à aucune des
// deux radios simulées.
SimRadio* radioFor(const char* which) {
  if (strcmp(which, "mesh") == 0) return &mesh_radio_driver;
  if (strcmp(which, "aprs") == 0) return &aprs_radio_driver;
  return nullptr;
}

// Table nom<->code pour "sim set mppt status <nom>" (cf. MPPT_CHG_ST_* dans
// lib/mpptChg/mpptChg.h) — noms alignés sur mpptChg::getStatusAsString().
struct MpptStateName { const char* name; uint16_t code; };
const MpptStateName kMpptStates[] = {
  {"night", MPPT_CHG_ST_NIGHT}, {"idle", MPPT_CHG_ST_IDLE}, {"vsrcv", MPPT_CHG_ST_VSRCV},
  {"scan", MPPT_CHG_ST_SCAN}, {"bulk", MPPT_CHG_ST_BULK}, {"absorb", MPPT_CHG_ST_ABSORB},
  {"float", MPPT_CHG_ST_FLOAT},
};

// Parse une chaîne hex ("A1B2C3"...) en octets. Espaces ignorés. Retourne le
// nombre d'octets écrits dans `out` (capacité `max`).
size_t parseHex(const char* s, uint8_t* out, size_t max) {
  size_t n = 0;
  int hi = -1;
  for (const char* p = s; *p && n < max; p++) {
    char c = *p;
    int v;
    if (c >= '0' && c <= '9') {
      v = c - '0';
    } else if (c >= 'a' && c <= 'f') {
      v = c - 'a' + 10;
    } else if (c >= 'A' && c <= 'F') {
      v = c - 'A' + 10;
    } else {
      continue;  // espaces, séparateurs — ignorés
    }
    if (hi < 0) {
      hi = v;
    } else {
      out[n++] = (uint8_t)((hi << 4) | v);
      hi = -1;
    }
  }
  return n;
}

void dumpStatus(Print& out) {
  auto& w = SimWorld::instance();
  std::lock_guard<std::mutex> lock(w.mutex);
  out.printf("--- Simulateur ---\n");
  out.printf("weather:  temp=%.1fC hum=%.1f%% press=%.1fhPa\n", w.temperature_c, w.humidity_pct, w.pressure_hpa);
  out.printf("battery:  %.0fmV %.0fmA\n", w.battery_mv, w.battery_ma);
  out.printf("board5v:  %.0fmV %.0fmA\n", w.board5v_mv, w.board5v_ma);
  out.printf("solar:    %.0fmV %.0fmA (ina)\n", w.solar_ina_mv, w.solar_ina_ma);
  out.printf("mppt:     %s battery=%.0fmV/%.0fmA solar=%.0fmV/%.0fmA alert=%s status=%s\n",
    w.mppt_present ? "présent" : "absent", w.mppt_battery_mv, w.mppt_battery_ma,
    w.mppt_solar_mv, w.mppt_solar_ma, w.mppt_alert ? "oui" : "non",
    mpptChg::getStatusAsString(w.mppt_charge_state));
  out.printf("victron:  %s soc=%.0f%% state=%d\n",
    w.victron_present ? "présent" : "absent", w.victron_soc_pct, w.victron_state);
  out.printf("tca9555:  output=0x%04X config=0x%04X\n", w.tca9555_output, w.tca9555_config);
  out.printf("radio mesh: %d trames loggées, %d en attente RX, cad=%s, bruit=%ddBm\n",
    (int)w.mesh_log.size(), (int)w.mesh_rx_queue.size(),
    mesh_radio_driver.getCadBusy() ? "occupé" : "libre", mesh_radio_driver.getNoiseFloor());
  out.printf("radio aprs: %d trames loggées, %d en attente RX, cad=%s, bruit=%ddBm\n",
    (int)w.aprs_log.size(), (int)w.aprs_rx_queue.size(),
    aprs_radio_driver.getCadBusy() ? "occupé" : "libre", aprs_radio_driver.getNoiseFloor());
  out.printf("radio fsk:  %s, %d trames loggées, %d en attente RX\n",
    w.fsk_mode ? "écoute WH65B active" : "hors cycle (LoRa APRS)", (int)w.fsk_log.size(), (int)w.fsk_rx_queue.size());
}

bool cmdSet(const char* args, Print& out) {
  char kind[16] = {0};
  if (sscanf(args, "%15s", kind) != 1) {
    out.println("Usage: sim set battery|solar|board5v|weather ...");
    return true;
  }
  const char* rest = args + strlen(kind);
  while (*rest == ' ') rest++;

  auto& w = SimWorld::instance();
  if (strcmp(kind, "battery") == 0) {
    float mv, ma;
    int n = sscanf(rest, "%f %f", &mv, &ma);
    if (n >= 1) {
      std::lock_guard<std::mutex> lock(w.mutex);
      w.battery_mv = mv;
      w.mppt_battery_mv = mv;
      if (n == 2) {
        w.battery_ma = ma;
        w.mppt_battery_ma = ma;
      }
      out.printf("battery = %.0f mV, %.0f mA\n", w.battery_mv, w.battery_ma);
    } else {
      out.println("Usage: sim set battery <mV> [<mA>]");
    }
  } else if (strcmp(kind, "solar") == 0) {
    float mv, ma;
    int n = sscanf(rest, "%f %f", &mv, &ma);
    if (n >= 1) {
      std::lock_guard<std::mutex> lock(w.mutex);
      w.solar_ina_mv = mv;
      w.mppt_solar_mv = mv;
      if (n == 2) {
        w.solar_ina_ma = ma;
        w.mppt_solar_ma = ma;
      }
      out.printf("solar = %.0f mV, %.0f mA\n", w.solar_ina_mv, w.solar_ina_ma);
    } else {
      out.println("Usage: sim set solar <mV> [<mA>]");
    }
  } else if (strcmp(kind, "board5v") == 0) {
    float mv, ma;
    int n = sscanf(rest, "%f %f", &mv, &ma);
    if (n >= 1) {
      std::lock_guard<std::mutex> lock(w.mutex);
      w.board5v_mv = mv;
      if (n == 2) {
        w.board5v_ma = ma;
      }
      out.printf("board5v = %.0f mV, %.0f mA\n", w.board5v_mv, w.board5v_ma);
    } else {
      out.println("Usage: sim set board5v <mV> [<mA>]");
    }
  } else if (strcmp(kind, "weather") == 0) {
    float t, h, p;
    if (sscanf(rest, "%f %f %f", &t, &h, &p) == 3) {
      std::lock_guard<std::mutex> lock(w.mutex);
      w.temperature_c = t;
      w.humidity_pct = h;
      w.pressure_hpa = p;
      out.printf("weather = %.1fC %.1f%% %.1fhPa\n", t, h, p);
    } else {
      out.println("Usage: sim set weather <tempC> <humPct> <hPa>");
    }
  } else if (strcmp(kind, "mppt") == 0) {
    char sub[16] = {0};
    char val[16] = {0};
    if (sscanf(rest, "%15s %15s", sub, val) == 2) {
      std::lock_guard<std::mutex> lock(w.mutex);
      if (strcmp(sub, "present") == 0) {
        w.mppt_present = (strcmp(val, "on") == 0);
        out.printf("mppt.present = %s\n", w.mppt_present ? "on" : "off");
      } else if (strcmp(sub, "alert") == 0) {
        w.mppt_alert = (strcmp(val, "on") == 0);
        out.printf("mppt.alert = %s\n", w.mppt_alert ? "on" : "off");
      } else if (strcmp(sub, "status") == 0) {
        bool found = false;
        for (auto& s : kMpptStates) {
          if (strcasecmp(val, s.name) == 0) {
            w.mppt_charge_state = s.code;
            found = true;
            break;
          }
        }
        if (found) {
          out.printf("mppt.status = %s\n", mpptChg::getStatusAsString(w.mppt_charge_state));
        } else {
          out.println("États: night|idle|vsrcv|scan|bulk|absorb|float");
        }
      } else {
        out.println("Usage: sim set mppt present|alert on|off  |  sim set mppt status <état>");
      }
    } else {
      out.println("Usage: sim set mppt present|alert on|off  |  sim set mppt status <état>");
    }
  } else if (strcmp(kind, "victron") == 0) {
    char sub[16] = {0};
    char val[16] = {0};
    if (sscanf(rest, "%15s %15s", sub, val) == 2) {
      std::lock_guard<std::mutex> lock(w.mutex);
      if (strcmp(sub, "present") == 0) {
        w.victron_present = (strcmp(val, "on") == 0);
        out.printf("victron.present = %s\n", w.victron_present ? "on" : "off");
      } else if (strcmp(sub, "soc") == 0) {
        w.victron_soc_pct = (float)atof(val);
        out.printf("victron.soc = %.0f%%\n", w.victron_soc_pct);
      } else if (strcmp(sub, "state") == 0) {
        w.victron_state = atoi(val);
        out.printf("victron.state = %d\n", w.victron_state);
      } else {
        out.println("Usage: sim set victron present on|off  |  sim set victron soc <pct>  |  sim set victron state <code>");
      }
    } else {
      out.println("Usage: sim set victron present on|off  |  sim set victron soc <pct>  |  sim set victron state <code>");
    }
  } else if (strcmp(kind, "noise") == 0) {
    char which[8] = {0};
    float rssi;
    if (sscanf(rest, "%7s %f", which, &rssi) == 2) {
      SimRadio* radio = radioFor(which);
      if (radio) {
        radio->setNoiseFloor((int)rssi);
        out.printf("noise.%s = %d dBm\n", which, (int)rssi);
      } else {
        out.println("Radio inconnue (mesh|aprs)");
      }
    } else {
      out.println("Usage: sim set noise mesh|aprs <rssi_dBm>");
    }
  } else {
    out.println("Clé inconnue (battery|solar|board5v|weather|mppt|victron|noise)");
  }
  return true;
}

bool cmdCad(const char* args, Print& out) {
  char which[8] = {0};
  char state[8] = {0};
  if (sscanf(args, "%7s %7s", which, state) != 2) {
    out.println("Usage: sim cad mesh|aprs on|off");
    return true;
  }
  SimRadio* radio = radioFor(which);
  if (!radio) {
    out.println("Radio inconnue (mesh|aprs)");
    return true;
  }
  bool busy = (strcmp(state, "on") == 0);
  radio->setCadBusy(busy);
  out.printf("cad.%s = %s (canal %s)\n", which, busy ? "on" : "off", busy ? "occupé" : "libre");
  return true;
}

bool cmdRx(const char* args, Print& out) {
  char which[8] = {0};
  if (sscanf(args, "%7s", which) != 1) {
    out.println("Usage: sim rx mesh|aprs|fsk [rssi <dBm>] [snr <dB>] <hex>|ascii <texte>");
    return true;
  }
  const char* rest = args + strlen(which);
  while (*rest == ' ') rest++;

  // Options "rssi <val>"/"snr <val>" optionnelles, dans n'importe quel ordre,
  // avant le payload — pas en position finale : le payload ascii (texte APRS
  // libre) pourrait sinon se terminer par des nombres et être tronqué par
  // erreur. Défauts = anciennes constantes globales de SimRadio (-90/8) pour
  // mesh/aprs si non précisé ; le fsk garde son propre défaut historique
  // (-55dBm, cf. ci-dessous) tant que rssi n'est pas explicitement donné.
  float rssi = -90.0f, snr = 8.0f;
  bool rssiSet = false;
  for (;;) {
    char kw[8] = {0};
    int consumed = 0;
    if (sscanf(rest, "%7s%n", kw, &consumed) != 1) break;
    float val;
    int consumed2 = 0;
    if (strcmp(kw, "rssi") == 0 && sscanf(rest + consumed, " %f%n", &val, &consumed2) == 1) {
      rssi = val;
      rssiSet = true;
    } else if (strcmp(kw, "snr") == 0 && sscanf(rest + consumed, " %f%n", &val, &consumed2) == 1) {
      snr = val;
    } else {
      break;
    }
    rest += consumed + consumed2;
    while (*rest == ' ') rest++;
  }

  uint8_t buf[256];
  size_t n;
  if (strncmp(rest, "ascii ", 6) == 0) {
    // Payload texte brut (ex: trame APRS lisible "N0CALL>APRS:!4903.50N/..."),
    // copié tel quel — pratique pour l'APRS, qui est un protocole texte.
    const char* text = rest + 6;
    size_t len = strlen(text);
    n = len < sizeof(buf) ? len : sizeof(buf);
    memcpy(buf, text, n);
  } else {
    n = parseHex(rest, buf, sizeof(buf));
  }
  if (n == 0) {
    out.println("Trame vide ou hex invalide");
    return true;
  }

  auto& w = SimWorld::instance();
  std::vector<uint8_t> pkt(buf, buf + n);
  if (strcmp(which, "mesh") == 0) {
    {
      std::lock_guard<std::mutex> lock(w.mutex);
      w.mesh_rx_queue.push_back({std::move(pkt), rssi, snr});
    }
    // Réveille immédiatement task_mesh.cpp (cf. core/RadioActivityNotify.h) —
    // sinon, depuis que cette tâche dort entre deux réveils radio au lieu de
    // faire du polling en vTaskDelay(1) (même changement que côté matériel
    // réel, ici juste déclenché par l'injection CLI/web plutôt qu'une IRQ),
    // la trame injectée ne serait vue qu'au prochain réveil périodique
    // (jusqu'à MESH_TASK_MAX_WAIT_MS).
    xTaskNotifyGive(g_mesh_task_handle);
    out.printf("Trame mesh injectée (%u octets, rssi=%.0fdBm, snr=%.1fdB)\n", (unsigned)n, rssi, snr);
  } else if (strcmp(which, "aprs") == 0) {
    // Préfixe avec le header LoRa-APRS 3 octets ('<' 0xFF 0x01, cf.
    // src/aprs/AprsEngine.h LORA_APRS_HEADER_*) : AprsEngine::onAprsPacketReceived
    // le vérifie et rejette silencieusement toute trame qui ne l'a pas — sans
    // ce préfixe la trame injectée serait juste loggée (radio_log) sans jamais
    // être réellement décodée (digipeat, query, message...).
    std::vector<uint8_t> framed;
    framed.reserve(n + 3);
    framed.push_back(0x3C);
    framed.push_back(0xFF);
    framed.push_back(0x01);
    framed.insert(framed.end(), pkt.begin(), pkt.end());

    {
      std::lock_guard<std::mutex> lock(w.mutex);
      w.aprs_rx_queue.push_back({std::move(framed), rssi, snr});
    }
    xTaskNotifyGive(g_aprs_task_handle);  // cf. commentaire équivalent ci-dessus (mesh)
    out.printf("Trame aprs injectée (%u octets + header LoRa-APRS, rssi=%.0fdBm, snr=%.1fdB)\n", (unsigned)n, rssi, snr);
  } else if (strcmp(which, "fsk") == 0) {
    float fskRssi = rssiSet ? rssi : -55.0f;  // défaut historique FSK, cf. AprsRadioHwSim.cpp
    std::lock_guard<std::mutex> lock(w.mutex);
    w.fsk_rx_queue.push_back({std::move(pkt), fskRssi, snr});
    out.printf("Trame WH65B (fsk) injectée (%u octets, rssi=%.0fdBm) — consommée au prochain cycle météo (taskWeather)\n", (unsigned)n, fskRssi);
  } else {
    out.println("Usage: sim rx mesh|aprs|fsk [rssi <dBm>] [snr <dB>] <hex>");
  }
  return true;
}

}  // namespace

bool simHandleCommand(const char* cmd, Print& out) {
  if (strncmp(cmd, "sim", 3) != 0 || (cmd[3] != '\0' && cmd[3] != ' ')) {
    return false;
  }
  const char* rest = cmd + 3;
  while (*rest == ' ') rest++;

  if (strcmp(rest, "status") == 0) {
    dumpStatus(out);
    return true;
  }
  if (strncmp(rest, "set ", 4) == 0) {
    return cmdSet(rest + 4, out);
  }
  if (strncmp(rest, "rx ", 3) == 0) {
    return cmdRx(rest + 3, out);
  }
  if (strncmp(rest, "cad ", 4) == 0) {
    return cmdCad(rest + 4, out);
  }

  out.println("Commandes sim: status, set battery|solar|board5v|weather|mppt|victron|noise ..., rx mesh|aprs|fsk <hex>|ascii <texte>, cad mesh|aprs on|off");
  return true;
}

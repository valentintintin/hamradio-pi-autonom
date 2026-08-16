#include "SimRadio.h"

#include <Arduino.h>
#include <cstring>
#include "core/Log.h"

int SimRadio::recvRaw(uint8_t* bytes, int sz) {
  auto& w = SimWorld::instance();
  RxQueueEntry entry;
  {
    std::lock_guard<std::mutex> lock(w.mutex);
    if (_rxQueue.empty()) {
      return 0;
    }
    entry = std::move(_rxQueue.front());
    _rxQueue.pop_front();
  }

  int n = (int)entry.data.size();
  if (n > sz) {
    n = sz;
  }
  memcpy(bytes, entry.data.data(), n);

  // RSSI/SNR portés par la trame injectée (cf. "sim rx ... rssi <dBm> snr
  // <dB>", SimCli.cpp) plutôt qu'une constante — permet de tester un paquet
  // à la limite du seuil de décodage sans état global partagé.
  _last_rssi = entry.rssi;
  _last_snr = entry.snr;

  {
    std::lock_guard<std::mutex> lock(w.mutex);
    RadioLogEntry e;
    e.tx = false;
    e.millis_ts = millis();
    e.data.assign(bytes, bytes + n);
    e.rssi = _last_rssi;
    e.snr = _last_snr;
    w.pushLog(_log, std::move(e));
  }

  _n_recv++;
  LOG_D(_tag, "RX simulé (%d octets, rssi=%.0fdBm, snr=%.1fdB)", n, _last_rssi, _last_snr);
  return n;
}

uint32_t SimRadio::getEstAirtimeFor(int len_bytes) {
  // Approximation grossière (pas de vrai calcul LoRa SF/BW) — suffisante pour
  // le scheduling CAD/dispatcher, qui n'a besoin que d'un ordre de grandeur.
  return 50 + (uint32_t)len_bytes * 2;
}

float SimRadio::packetScore(float snr, int /*packet_len*/) {
  return snr;
}

bool SimRadio::startSendRaw(const uint8_t* bytes, int len) {
  auto& w = SimWorld::instance();
  {
    std::lock_guard<std::mutex> lock(w.mutex);
    w.logTx(_log, bytes, len);
  }
  LOG_D(_tag, "TX simulé (%d octets)", len);
  _n_sent++;
  _sending = true;
  _send_done_at = millis() + getEstAirtimeFor(len);
  return true;
}

bool SimRadio::isSendComplete() {
  if (!_sending) {
    return true;
  }
  if ((long)(millis() - _send_done_at) >= 0) {
    _sending = false;
    return true;
  }
  return false;
}

void SimRadio::onSendFinished() {}

void SimRadio::setParams(float freq, float bw, uint8_t sf, uint8_t cr) {
  _freq = freq;
  _bw = bw;
  _sf = sf;
  _cr = cr;
  LOG_I(_tag, "setParams(%.3f, %.1f, %d, %d) — appliqué en simulé (pas de vraie RF)", freq, bw, sf, cr);
}

void SimRadio::setTxPower(int8_t dbm) {
  _tx_power_dbm = dbm;
  LOG_I(_tag, "setTxPower(%d) — appliqué en simulé (pas de vraie RF)", dbm);
}

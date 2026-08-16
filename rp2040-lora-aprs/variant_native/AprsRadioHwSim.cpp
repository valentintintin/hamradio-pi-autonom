#include "AprsRadioHwSim.h"
#include "SimWorld.h"
#include <FineOffsetWH65B.h>
#include "core/Log.h"
#include <Arduino.h>
#include <cstring>
#include <mutex>
#include <vector>

#define TAG "APRS-RADIO"

bool AprsRadioHwSim::switchToFsk() {
  SimWorld::instance().fsk_mode = true;
  LOG_D(TAG, "FSK simulé : injectez une trame WH65B via 'sim rx fsk <hex>' ou le dashboard web");
  return true;
}

bool AprsRadioHwSim::switchToLora() {
  SimWorld::instance().fsk_mode = false;
  return true;
}

bool AprsRadioHwSim::receiveWh65bFrame(uint32_t timeoutMs, uint8_t* outBuf, float* outRssi) {
  auto& w = SimWorld::instance();
  unsigned long start = millis();

  for (;;) {
    RxQueueEntry entry;
    bool got = false;
    {
      std::lock_guard<std::mutex> lock(w.mutex);
      if (!w.fsk_rx_queue.empty()) {
        entry = std::move(w.fsk_rx_queue.front());
        w.fsk_rx_queue.pop_front();
        got = true;
      }
    }

    if (got) {
      memset(outBuf, 0, WH65B_PAYLOAD_LEN);
      size_t n = entry.data.size() < (size_t)WH65B_PAYLOAD_LEN ? entry.data.size() : (size_t)WH65B_PAYLOAD_LEN;
      memcpy(outBuf, entry.data.data(), n);

      {
        std::lock_guard<std::mutex> lock(w.mutex);
        RadioLogEntry e;
        e.tx = false;
        e.millis_ts = millis();
        e.data.assign(outBuf, outBuf + WH65B_PAYLOAD_LEN);
        // RSSI porté par la trame injectée ("sim rx fsk rssi <dBm> ...", cf.
        // SimCli.cpp) — -55dBm par défaut si non précisé, pas de concept de
        // SNR pour le FSK/WH65B.
        e.rssi = entry.rssi;
        w.pushLog(w.fsk_log, std::move(e));
      }

      if (outRssi) {
        *outRssi = entry.rssi;
      }
      return true;
    }

    if (millis() - start > timeoutMs) {
      return false;
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

bool AprsRadioHwSim::relayWh65bFrame(const uint8_t* data, size_t len, int8_t /*powerDbm*/) {
  SimWorld::instance().logTx(SimWorld::instance().fsk_log, data, (int)len);
  return true;
}

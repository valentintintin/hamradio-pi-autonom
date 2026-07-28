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
    std::vector<uint8_t> pkt;
    {
      std::lock_guard<std::mutex> lock(w.mutex);
      if (!w.fsk_rx_queue.empty()) {
        pkt = std::move(w.fsk_rx_queue.front());
        w.fsk_rx_queue.pop_front();
      }
    }

    if (!pkt.empty()) {
      memset(outBuf, 0, WH65B_PAYLOAD_LEN);
      size_t n = pkt.size() < (size_t)WH65B_PAYLOAD_LEN ? pkt.size() : (size_t)WH65B_PAYLOAD_LEN;
      memcpy(outBuf, pkt.data(), n);

      {
        std::lock_guard<std::mutex> lock(w.mutex);
        RadioLogEntry e;
        e.tx = false;
        e.millis_ts = millis();
        e.data.assign(outBuf, outBuf + WH65B_PAYLOAD_LEN);
        e.rssi = -55.0f;
        w.pushLog(w.fsk_log, std::move(e));
      }

      if (outRssi) {
        *outRssi = -55.0f;
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

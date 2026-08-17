#include "AprsDispatcher.h"
#include "core/Log.h"
#include "core/LockGuard.h"
#include "core/RadioActivityNotify.h"
#include <string.h>

#define TAG "APRS-DSP"

AprsDispatcher::AprsDispatcher(mesh::Radio& radio, mesh::MillisecondClock& ms)
  : _radio(&radio), _ms(&ms), _rx_callback(nullptr),
    _outbound_active(false), _outbound_len(0),
    _cad_busy_start(0), _next_tx_time(0),
    _paused(false),
    _total_air_time(0), _n_sent(0), _n_recv(0)
{
  memset(_tx_pool, 0, sizeof(_tx_pool));
  _pool_mutex = xSemaphoreCreateMutex();
}

void AprsDispatcher::begin() {
  _next_tx_time = _ms->getMillis();
  _n_sent = _n_recv = 0;

  _radio->begin();
}

void AprsDispatcher::pause() {
  _paused = true;
  LOG_D(TAG, "Pause demandée");

  unsigned long timeout = _ms->getMillis() + 10000;
  while (_outbound_active && (long)(_ms->getMillis() - timeout) < 0) {
    _radio->loop();
    if (_radio->isSendComplete()) {
      _radio->onSendFinished();
      _outbound_active = false;
    }
    delay(1);
  }
  if (_outbound_active) {
    LOG_W(TAG, "Timeout attente fin TX, forcé");
  }
  _outbound_active = false;
}

void AprsDispatcher::resume() {
  _radio->begin();
  _paused = false;
  LOG_D(TAG, "Reprise");
}

void AprsDispatcher::loop() {
  if (_paused) {
    return;
  }

  _radio->loop();

  if (_outbound_active) {
    if (_radio->isSendComplete()) {
      long t = _ms->getMillis() - _outbound_start;
      _total_air_time += t;
      _next_tx_time = _ms->getMillis();

      _radio->onSendFinished();
      _outbound_active = false;
      _n_sent++;
    } else if (millisHasNowPassed(_outbound_expiry)) {
      LOG_W(TAG, "TX timeout, abandon");
      _radio->onSendFinished();
      _outbound_active = false;
    } else {
      return;
    }
  }

  checkRecv();
  checkSend();
}

void AprsDispatcher::checkRecv() {
  uint8_t raw[APRS_MAX_PACKET_SIZE];
  int len = _radio->recvRaw(raw, APRS_MAX_PACKET_SIZE);

  if (len > 0 && _rx_callback) {
    _n_recv++;
    float rssi = _radio->getLastRSSI();
    float snr = _radio->getLastSNR();
    uint32_t airtime = _radio->getEstAirtimeFor(len);
    _total_air_time += airtime;

    _rx_callback->onAprsPacketReceived(raw, len, rssi, snr);
  }
}

void AprsDispatcher::checkSend() {
  if (getOutboundCount(_ms->getMillis()) == 0) {
    return;
  }

  if (!millisHasNowPassed(_next_tx_time)) {
    return;
  }

  if (_radio->isReceiving()) {
    if (_cad_busy_start == 0) {
      _cad_busy_start = _ms->getMillis();
    }
    if (_ms->getMillis() - _cad_busy_start > getCADFailMaxDuration()) {
      // channel busy trop longtemps : on force l'envoi
    } else {
      _next_tx_time = futureMillis(getCADFailRetryDelay());
      return;
    }
  }
  _cad_busy_start = 0;

  AprsQueuedPacket* pkt = getNextOutbound(_ms->getMillis());
  if (!pkt) {
    return;
  }

  memcpy(_outbound_data, pkt->data, pkt->len);
  _outbound_len = pkt->len;
  pkt->used = false;

  uint32_t max_airtime = _radio->getEstAirtimeFor(_outbound_len) * 3 / 2;
  _outbound_start = _ms->getMillis();

  bool success = _radio->startSendRaw(_outbound_data, _outbound_len);
  if (!success) {
    LOG_E(TAG, "startSendRaw FAIL %d bytes", _outbound_len);
    _outbound_active = false;
    return;
  }

  _outbound_active = true;
  _outbound_expiry = futureMillis(max_airtime);
}

bool AprsDispatcher::send(const uint8_t* data, uint8_t len, uint8_t priority, uint32_t delay_ms) {
  if (len == 0 || len > APRS_MAX_PACKET_SIZE) {
    return false;
  }

  LockGuard lock(_pool_mutex);

  AprsQueuedPacket* slot = allocSlot();
  if (!slot) {
    LOG_W(TAG, "Queue TX pleine, paquet perdu (%d bytes)", len);
    return false;
  }

  memcpy(slot->data, data, len);
  slot->len = len;
  slot->priority = priority;
  slot->send_after = futureMillis(delay_ms);
  slot->used = true;

  // Sur RP2040, xTaskNotifyGive avec handle nul déclenche un configASSERT
  // fatal (contrairement au shim natif, qui ignore silencieusement) : send()
  // peut être appelé avant que taskAprsLoop() ait renseigné ce handle.
  if (g_aprs_task_handle) {
    xTaskNotifyGive(g_aprs_task_handle);
  }

  return true;
}

AprsQueuedPacket* AprsDispatcher::allocSlot() {
  for (int i = 0; i < APRS_TX_QUEUE_SIZE; i++) {
    if (!_tx_pool[i].used) {
      return &_tx_pool[i];
    }
  }
  return nullptr;
}

AprsQueuedPacket* AprsDispatcher::getNextOutbound(unsigned long now) {
  AprsQueuedPacket* best = nullptr;

  for (int i = 0; i < APRS_TX_QUEUE_SIZE; i++) {
    if (!_tx_pool[i].used) {
      continue;
    }
    if (!millisHasNowPassed(_tx_pool[i].send_after)) {
      continue;
    }

    if (!best || _tx_pool[i].priority < best->priority) {
      best = &_tx_pool[i];
    }
  }
  return best;
}

int AprsDispatcher::getOutboundCount(unsigned long now) const {
  int count = 0;
  for (int i = 0; i < APRS_TX_QUEUE_SIZE; i++) {
    if (_tx_pool[i].used && millisHasNowPassed(_tx_pool[i].send_after)) {
      count++;
    }
  }
  return count;
}

bool AprsDispatcher::millisHasNowPassed(unsigned long timestamp) const {
  return (long)(_ms->getMillis() - timestamp) > 0;
}

unsigned long AprsDispatcher::futureMillis(int millis_from_now) const {
  return _ms->getMillis() + millis_from_now;
}

uint32_t AprsDispatcher::msUntilNextAction(unsigned long now) const {
  if (_outbound_active) {
    long remaining = (long)(_outbound_expiry - now);
    return remaining > 0 ? (uint32_t)remaining : 0;
  }

  bool any = false;
  unsigned long earliest = 0;
  for (int i = 0; i < APRS_TX_QUEUE_SIZE; i++) {
    if (_tx_pool[i].used && (!any || (long)(_tx_pool[i].send_after - earliest) < 0)) {
      earliest = _tx_pool[i].send_after;
      any = true;
    }
  }

  if (!any) {
    return 0xFFFFFFFFu;
  }

  if ((long)(_next_tx_time - earliest) > 0) {
    earliest = _next_tx_time;
  }

  long remaining = (long)(earliest - now);
  return remaining > 0 ? (uint32_t)remaining : 0;
}

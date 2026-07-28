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

// ============================================================================
// Pause/resume — laisser la radio à une autre tâche
// ============================================================================
void AprsDispatcher::pause() {
  _paused = true;
  LOG_D(TAG, "Pause demandée");

  // Attendre la fin d'un éventuel TX en cours
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

// ============================================================================
// Loop principal — appelé depuis la task FreeRTOS
// ============================================================================
void AprsDispatcher::loop() {
  if (_paused) {
    return;
  }

  _radio->loop();

  // Si un envoi est en cours, attendre qu'il finisse
  if (_outbound_active) {
    if (_radio->isSendComplete()) {
      long t = _ms->getMillis() - _outbound_start;
      _total_air_time += t;

      // Pas de budget duty cycle à recharger (bande amateur, cf. AprsDispatcher.h) :
      // le prochain envoi n'est retardé que par le CAD, il peut donc démarrer tout de suite.
      _next_tx_time = _ms->getMillis();

      _radio->onSendFinished();
      _outbound_active = false;
      _n_sent++;
    } else if (millisHasNowPassed(_outbound_expiry)) {
      // Timeout envoi
      LOG_W(TAG, "TX timeout, abandon");
      _radio->onSendFinished();
      _outbound_active = false;
    } else {
      return; // envoi en cours, on ne peut rien faire d'autre
    }
  }

  checkRecv();
  checkSend();
}

// ============================================================================
// Réception
// ============================================================================
void AprsDispatcher::checkRecv() {
  uint8_t raw[APRS_MAX_PACKET_SIZE];
  int len = _radio->recvRaw(raw, APRS_MAX_PACKET_SIZE);

  if (len > 0 && _rx_callback) {
    _n_recv++;
    float rssi = _radio->getLastRSSI();
    float snr = _radio->getLastSNR();
    uint32_t airtime = _radio->getEstAirtimeFor(len);
    _total_air_time += airtime; // comptabiliser le RX aussi (stat informative)

    _rx_callback->onAprsPacketReceived(raw, len, rssi, snr);
  }
}

// ============================================================================
// Émission — logique CAD (portée de MeshCore, sans plafond duty cycle)
// ============================================================================
void AprsDispatcher::checkSend() {
  if (getOutboundCount(_ms->getMillis()) == 0) {
    return;
  }

  if (!millisHasNowPassed(_next_tx_time)) {
    return;
  }

  // CAD — Channel Activity Detection
  if (_radio->isReceiving()) {
    if (_cad_busy_start == 0) {
      _cad_busy_start = _ms->getMillis();
    }
    if (_ms->getMillis() - _cad_busy_start > getCADFailMaxDuration()) {
      // Channel busy trop longtemps, forcer l'envoi
    } else {
      _next_tx_time = futureMillis(getCADFailRetryDelay());
      return;
    }
  }
  _cad_busy_start = 0;

  // Récupérer le prochain paquet prêt
  AprsQueuedPacket* pkt = getNextOutbound(_ms->getMillis());
  if (!pkt) {
    return;
  }

  // Copier et libérer le slot
  memcpy(_outbound_data, pkt->data, pkt->len);
  _outbound_len = pkt->len;
  pkt->used = false;

  // Envoyer
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

// ============================================================================
// API publique — enqueue un paquet
//
// Peut être appelé depuis plusieurs tâches productrices en même temps
// (beacon, CLI, mesh, et le traitement RX du dispatcher lui-même) : le verrou
// couvre tout le cycle allocSlot()+écriture pour qu'un seul producteur à la
// fois puisse choisir puis remplir un slot libre (sinon deux producteurs
// pourraient sélectionner le même slot avant que l'un des deux ne le marque
// "used", et l'un écraserait le paquet de l'autre).
// ============================================================================
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

  // Réveille la tâche APRS immédiatement (elle dort peut-être en attendant un
  // événement radio ou un paquet, cf. tasks/task_aprs.cpp) plutôt que de la
  // laisser attendre son prochain réveil planifié pour remarquer ce nouveau
  // paquet. Garde nulle nécessaire : send() peut être appelée avant que
  // taskAprsLoop() ait démarré et renseigné ce handle (ex: un beacon envoyé
  // très tôt au boot) — le vrai FreeRTOS (configASSERT sur RP2040) fait un
  // rtosFatalError() si on l'appelle avec un handle nul, contrairement au
  // shim natif qui l'ignore silencieusement.
  if (g_aprs_task_handle) {
    xTaskNotifyGive(g_aprs_task_handle);
  }

  return true;
}

// ============================================================================
// Queue helpers
// ============================================================================
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

// ============================================================================
// Millis helpers (gestion du rollover unsigned, copiées de MeshCore)
// ============================================================================
bool AprsDispatcher::millisHasNowPassed(unsigned long timestamp) const {
  return (long)(_ms->getMillis() - timestamp) > 0;
}

unsigned long AprsDispatcher::futureMillis(int millis_from_now) const {
  return _ms->getMillis() + millis_from_now;
}

// ============================================================================
// Combien de temps avant que ce dispatcher n'ait besoin d'être rappelé — cf.
// commentaire dans AprsDispatcher.h. Ne verrouille pas _pool_mutex : lu depuis
// tasks/task_aprs.cpp juste après loop() (qui l'a déjà pris/relâché), une
// lecture legèrement périmée ici ne fait au pire dormir un peu plus/moins
// longtemps que l'idéal, jamais rater un événement (le réveil radio ou
// xTaskNotifyGive() dans send() couvrent toujours le cas exact).
// ============================================================================
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
    return 0xFFFFFFFFu;  // rien en attente : le prochain réveil viendra d'une IRQ radio ou d'un send()
  }

  // Un retry CAD retarde peut-être ce paquet au-delà de son propre send_after.
  if ((long)(_next_tx_time - earliest) > 0) {
    earliest = _next_tx_time;
  }

  long remaining = (long)(earliest - now);
  return remaining > 0 ? (uint32_t)remaining : 0;
}

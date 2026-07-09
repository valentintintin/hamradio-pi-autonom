#pragma once

// ============================================================================
// AprsDispatcher — Dispatcher APRS inspiré de MeshCore Dispatcher
//
// Gère la queue TX/RX, le duty cycle, le CAD et le scheduling
// pour des paquets APRS bruts (pas le format MeshCore).
// Utilise mesh::Radio comme abstraction radio (réutilise le wrapper RadioLib).
// ============================================================================

#include <Dispatcher.h>  // pour mesh::Radio, mesh::MillisecondClock
#include <stdint.h>

// Taille max d'un paquet APRS LoRa (3 bytes header + 253 payload)
#define APRS_MAX_PACKET_SIZE    256
#define APRS_TX_QUEUE_SIZE      16
#define APRS_RX_QUEUE_SIZE      8

// Priorités
#define APRS_PRIO_ACK           0   // ACKs en premier
#define APRS_PRIO_DIGIPEAT      1   // Digipeated packets
#define APRS_PRIO_BEACON        2   // Beacons périodiques
#define APRS_PRIO_LOW           3   // Telemetry etc.

// ============================================================================
// Paquet APRS dans la queue
// ============================================================================
struct AprsQueuedPacket {
  uint8_t data[APRS_MAX_PACKET_SIZE];
  uint8_t len;
  uint8_t priority;
  uint32_t send_after;    // timestamp millis() min avant envoi
  bool     used;          // slot occupé
};

// ============================================================================
// Callback pour les paquets reçus
// ============================================================================
class AprsRxCallback {
public:
  virtual void onAprsPacketReceived(const uint8_t* data, uint8_t len, float rssi, float snr) = 0;
};

// ============================================================================
// AprsDispatcher
// ============================================================================
class AprsDispatcher {
public:
  AprsDispatcher(mesh::Radio& radio, mesh::MillisecondClock& ms);

  void begin();
  void loop();

  // Enqueue un paquet APRS brut
  bool send(const uint8_t* data, uint8_t len, uint8_t priority, uint32_t delay_ms = 0);

  // Callback RX
  void setRxCallback(AprsRxCallback* cb) { _rx_callback = cb; }

  // Pause/resume pour laisser la radio à une autre tâche (ex: FSK WH65B)
  // pause() attend la fin d'un TX en cours puis coupe la réception
  // resume() relance la réception LoRa
  void pause();
  void resume();
  bool isPaused() const { return _paused; }

  // Stats
  uint32_t getTotalAirTime() const { return _total_air_time; }
  uint32_t getPacketsSent() const { return _n_sent; }
  uint32_t getPacketsReceived() const { return _n_recv; }
  unsigned long getRemainingTxBudget() const { return _tx_budget_ms; }

private:
  mesh::Radio* _radio;
  mesh::MillisecondClock* _ms;
  AprsRxCallback* _rx_callback;

  // Queue TX (pool statique, triée par priorité)
  AprsQueuedPacket _tx_pool[APRS_TX_QUEUE_SIZE];

  // Paquet en cours d'envoi
  uint8_t _outbound_data[APRS_MAX_PACKET_SIZE];
  uint8_t _outbound_len;
  bool    _outbound_active;
  unsigned long _outbound_start;
  unsigned long _outbound_expiry;

  // Duty cycle tracking (inspiré MeshCore Dispatcher)
  unsigned long _tx_budget_ms;
  unsigned long _last_budget_update;
  unsigned long _duty_cycle_window_ms;
  float _duty_cycle;

  // CAD state
  unsigned long _cad_busy_start;
  unsigned long _next_tx_time;

  // Pause
  volatile bool _paused;

  // Stats
  unsigned long _total_air_time;
  uint32_t _n_sent;
  uint32_t _n_recv;

  // --- Méthodes internes (logique portée de MeshCore) ----------------------

  void checkRecv();
  void checkSend();
  void updateTxBudget();

  // Retransmit delay randomisé — MeshCore: rand(0, 5*t+1) ou t = airtime * factor
  uint32_t getRetransmitDelay(uint32_t airtime_ms);

  // CAD retry delay
  uint32_t getCADFailRetryDelay() const { return 200; }
  uint32_t getCADFailMaxDuration() const { return 4000; }

  // Duty cycle : 1% par défaut (fenêtre 1h)
  float getAirtimeBudgetFactor() const { return 99.0f; }  // 1/(1+99) = 1%

  // Queue helpers
  AprsQueuedPacket* allocSlot();
  AprsQueuedPacket* getNextOutbound(unsigned long now);
  int getOutboundCount(unsigned long now) const;

  // Millis helpers (gestion du rollover, copiées de MeshCore)
  bool millisHasNowPassed(unsigned long timestamp) const;
  unsigned long futureMillis(int millis_from_now) const;
};

#pragma once

// Pas de plafond duty cycle : bande amateur, contrairement aux bandes ISM/SRD.
// Seul le CAD (écoute avant transmission) limite l'émission.

#include <Dispatcher.h>
#include <stdint.h>
#include <FreeRTOS.h>
#include <semphr.h>

#define APRS_MAX_PACKET_SIZE    256
#define APRS_TX_QUEUE_SIZE      16
#define APRS_RX_QUEUE_SIZE      8

#define APRS_PRIO_ACK_MESSAGE   0
#define APRS_PRIO_DIGIPEAT      1
#define APRS_PRIO_BEACON        2
#define APRS_PRIO_LOW           3

struct AprsQueuedPacket {
  uint8_t data[APRS_MAX_PACKET_SIZE];
  uint8_t len;
  uint8_t priority;
  uint32_t send_after;
  bool     used;
};

class AprsRxCallback {
public:
  virtual void onAprsPacketReceived(const uint8_t* data, uint8_t len, float rssi, float snr) = 0;
};

class AprsDispatcher {
public:
  AprsDispatcher(mesh::Radio& radio, mesh::MillisecondClock& ms);

  void begin();
  void loop();

  bool send(const uint8_t* data, uint8_t len, uint8_t priority, uint32_t delay_ms = 0);

  void setRxCallback(AprsRxCallback* cb) { _rx_callback = cb; }

  // pause() attend la fin d'un TX en cours puis coupe la réception (laisse
  // la radio à une autre tâche, ex: FSK WH65B) ; resume() relance en LoRa.
  void pause();
  void resume();
  bool isPaused() const { return _paused; }

  uint32_t getTotalAirTime() const { return _total_air_time; }
  uint32_t getPacketsSent() const { return _n_sent; }
  uint32_t getPacketsReceived() const { return _n_recv; }

  uint32_t msUntilNextAction(unsigned long now) const;

private:
  mesh::Radio* _radio;
  mesh::MillisecondClock* _ms;
  AprsRxCallback* _rx_callback;

  // Mutex : send() peut être appelé concurremment par plusieurs tâches
  // (beacon, CLI, mesh, RX du dispatcher) ; protège allocSlot()+écriture
  // contre deux producteurs choisissant le même slot libre.
  AprsQueuedPacket _tx_pool[APRS_TX_QUEUE_SIZE];
  SemaphoreHandle_t _pool_mutex;

  uint8_t _outbound_data[APRS_MAX_PACKET_SIZE];
  uint8_t _outbound_len;
  bool    _outbound_active;
  unsigned long _outbound_start;
  unsigned long _outbound_expiry;

  unsigned long _cad_busy_start;
  unsigned long _next_tx_time;

  volatile bool _paused;

  unsigned long _total_air_time;
  uint32_t _n_sent;
  uint32_t _n_recv;

  void checkRecv();
  void checkSend();

  uint32_t getCADFailRetryDelay() const { return 200; }
  uint32_t getCADFailMaxDuration() const { return 4000; }

  AprsQueuedPacket* allocSlot();
  AprsQueuedPacket* getNextOutbound(unsigned long now);
  int getOutboundCount(unsigned long now) const;

  bool millisHasNowPassed(unsigned long timestamp) const;
  unsigned long futureMillis(int millis_from_now) const;
};

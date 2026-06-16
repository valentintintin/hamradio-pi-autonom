#pragma once

#include <Dispatcher.h>
#include <aprs.hpp>

namespace mine {

class MyPacketManager {
public:
  virtual aprs::packet* allocNew() = 0;
  virtual void free(aprs::packet* packet) = 0;

  virtual void queueOutbound(aprs::packet* packet, uint8_t priority, uint32_t scheduled_for) = 0;
  virtual aprs::packet* getNextOutbound(uint32_t now) = 0;    // by priority
  virtual int getOutboundCount(uint32_t now) const = 0;
  virtual int getOutboundTotal() const = 0;
  virtual int getFreeCount() const = 0;
  virtual aprs::packet* getOutboundByIdx(int i) = 0;
  virtual aprs::packet* removeOutboundByIdx(int i) = 0;
  virtual void queueInbound(aprs::packet* packet, uint32_t scheduled_for) = 0;
  virtual aprs::packet* getNextInbound(uint32_t now) = 0;
};

/**
 * \brief  The low-level task that manages detecting incoming Packets, and the queueing
 *      and scheduling of outbound Packets.
 */
class MyDispatcher {
  aprs::packet* outbound;  // current outbound packet
  unsigned long outbound_expiry, outbound_start, total_air_time, rx_air_time;
  unsigned long next_tx_time;
  unsigned long cad_busy_start;
  unsigned long radio_nonrx_start;
  unsigned long next_floor_calib_time, next_agc_reset_time;
  bool  prev_isrecv_mode;
  uint32_t n_sent_flood, n_sent_direct;
  uint32_t n_recv_flood, n_recv_direct;

  void processRecvPacket(aprs::packet* pkt);

protected:
  MyPacketManager * _mgr;
  mesh::Radio * _radio;
  mesh::MillisecondClock * _ms;
  uint16_t _err_flags;

  MyDispatcher(mesh::Radio & radio, mesh::MillisecondClock & ms, mesh::PacketManager & mgr)
    : _radio(&radio), _ms(&ms), _mgr(&mgr)
  {
    outbound = NULL;
    total_air_time = rx_air_time = 0;
    next_tx_time = ms.getMillis();
    cad_busy_start = 0;
    next_floor_calib_time = next_agc_reset_time = 0;
    _err_flags = 0;
    radio_nonrx_start = 0;
    prev_isrecv_mode = true;
  }

  virtual mesh::DispatcherAction onRecvPacket(aprs::packet* pkt) = 0;

  virtual void logRxRaw(float snr, float rssi, const uint8_t raw[], int len) { }   // custom hook

  virtual void logRx(aprs::packet* packet, int len, float score) { }   // hooks for custom logging
  virtual void logTx(aprs::packet* packet, int len) { }
  virtual void logTxFail(aprs::packet* packet, int len) { }
  virtual const char* getLogDateTime() { return ""; }

  virtual float getAirtimeBudgetFactor() const;
  virtual int calcRxDelay(float score, uint32_t air_time) const;
  virtual uint32_t getCADFailRetryDelay() const;
  virtual uint32_t getCADFailMaxDuration() const;
  virtual int getInterferenceThreshold() const { return 0; }    // disabled by default
  virtual int getAGCResetInterval() const { return 0; }    // disabled by default

public:
  void begin();
  void loop();

  aprs::packet* obtainNewPacket();
  void releasePacket(aprs::packet* packet);
  void sendPacket(aprs::packet* packet, uint8_t priority, uint32_t delay_millis=0);

  unsigned long getTotalAirTime() const { return total_air_time; }
  unsigned long getReceiveAirTime() const {return rx_air_time; }
  uint32_t getNumSentFlood() const { return n_sent_flood; }
  uint32_t getNumSentDirect() const { return n_sent_direct; }
  uint32_t getNumRecvFlood() const { return n_recv_flood; }
  uint32_t getNumRecvDirect() const { return n_recv_direct; }
  void resetStats() {
    n_sent_flood = n_sent_direct = n_recv_flood = n_recv_direct = 0;
    _err_flags = 0;
  }

  // helper methods
  bool millisHasNowPassed(unsigned long timestamp) const;
  unsigned long futureMillis(int millis_from_now) const;

  bool tryParsePacket(aprs::packet* pkt, const uint8_t* raw, int len);

private:
  void checkRecv();
  void checkSend();
};

}
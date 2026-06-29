#pragma once

#include <Dispatcher.h>
#include <Aprs.h>

namespace mine {

struct AprsPacket {
  aprs::Packet packet{};
  uint8_t raw[aprs::kMaxPacketLength]{};
  uint16_t size = 0;
  int8_t snr = -99;
};

static const char* APRS_MY_CALL = "F4HVV-15";
static const char* ALIASES[] = {"RATZ"};
static const char* PREFIXES[] = {"WIDE"};

static aprs::DigipeaterOptions makeOptions() {
  aprs::DigipeaterOptions opt;
  opt.aliases = ALIASES;
  opt.aliasCount = sizeof(ALIASES) / sizeof(ALIASES[0]);
  opt.prefixes = PREFIXES;
  opt.prefixCount = sizeof(PREFIXES) / sizeof(PREFIXES[0]);
  return opt;
}

class MyAprsPacketManager {
public:
  virtual AprsPacket* allocNew() = 0;
  virtual void free(AprsPacket* packet) = 0;

  virtual void queueOutbound(AprsPacket* packet, uint8_t priority, uint32_t scheduled_for) = 0;
  virtual AprsPacket* getNextOutbound(uint32_t now) = 0;    // by priority
  virtual int getOutboundCount(uint32_t now) const = 0;
  virtual int getOutboundTotal() const = 0;
  virtual int getFreeCount() const = 0;
  virtual AprsPacket* getOutboundByIdx(int i) = 0;
  virtual AprsPacket* removeOutboundByIdx(int i) = 0;
  virtual void queueInbound(AprsPacket* packet, uint32_t scheduled_for) = 0;
  virtual AprsPacket* getNextInbound(uint32_t now) = 0;
};

class MyAprsDispatcher {
  AprsPacket* outbound;  // current outbound packet
  unsigned long outbound_expiry, outbound_start, total_air_time, rx_air_time;
  unsigned long next_tx_time;
  unsigned long cad_busy_start;
  unsigned long radio_nonrx_start;
  unsigned long next_floor_calib_time, next_agc_reset_time;
  bool  prev_isrecv_mode;
  uint32_t n_sent;
  uint32_t n_recv;

  void processRecvPacket(AprsPacket* pkt);

protected:
  MyAprsPacketManager * _mgr;
  mesh::Radio * _radio;
  mesh::MillisecondClock * _ms;
  uint16_t _err_flags;

  MyAprsDispatcher(mesh::Radio & radio, mesh::MillisecondClock & ms, MyAprsPacketManager & mgr)
    : _radio(&radio), _ms(&ms), _mgr(&mgr)
  {
    outbound = nullptr;
    total_air_time = rx_air_time = 0;
    next_tx_time = ms.getMillis();
    cad_busy_start = 0;
    next_floor_calib_time = next_agc_reset_time = 0;
    _err_flags = 0;
    radio_nonrx_start = 0;
    prev_isrecv_mode = true;
    n_sent = n_recv = 0;
    outbound_expiry = outbound_start = 0;
  }

  virtual mesh::DispatcherAction onRecvPacket(AprsPacket* pkt) = 0;

  virtual void logRxRaw(float snr, float rssi, const uint8_t raw[], int len) { }   // custom hook

  virtual void logRx(AprsPacket* packet, float score) { }   // hooks for custom logging
  virtual void logTx(AprsPacket* packet) { }
  virtual void logTxFail(AprsPacket* packet) { }
  virtual const char* getLogDateTime() { return ""; }

  virtual int calcRxDelay(float score, uint32_t air_time) const;
  virtual uint32_t getCADFailRetryDelay() const;
  virtual uint32_t getCADFailMaxDuration() const;
  virtual int getInterferenceThreshold() const { return 0; }    // disabled by default
  virtual int getAGCResetInterval() const { return 0; }    // disabled by default

public:
  void begin();
  void loop();

  AprsPacket* obtainNewPacket();
  void releasePacket(AprsPacket* packet);
  void sendPacket(AprsPacket* packet, uint8_t priority, uint32_t delay_millis=0);

  unsigned long getTotalAirTime() const { return total_air_time; }
  unsigned long getReceiveAirTime() const {return rx_air_time; }
  uint32_t getNumSent() const { return n_sent; }
  uint32_t getNumRecv() const { return n_recv; }
  void resetStats() {
    n_sent = n_recv = 0;
    _err_flags = 0;
  }

  // helper methods
  bool millisHasNowPassed(unsigned long timestamp) const;
  unsigned long futureMillis(int millis_from_now) const;

  bool tryParsePacket(AprsPacket* pkt, const uint8_t* raw, int len);

private:
  void checkRecv();
  void checkSend();
};

}
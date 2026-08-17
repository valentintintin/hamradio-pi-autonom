#pragma once

#include <Dispatcher.h>
#include <deque>
#include <vector>
#include "SimWorld.h"

class SimRadio : public mesh::Radio {
public:
  SimRadio(std::deque<RadioLogEntry>& log, std::deque<RxQueueEntry>& rxQueue, const char* tag)
    : _log(log), _rxQueue(rxQueue), _tag(tag) {}

  int recvRaw(uint8_t* bytes, int sz) override;
  uint32_t getEstAirtimeFor(int len_bytes) override;
  float packetScore(float snr, int packet_len) override;
  bool startSendRaw(const uint8_t* bytes, int len) override;
  bool isSendComplete() override;
  void onSendFinished() override;
  bool isInRecvMode() const override { return true; }
  float getLastRSSI() const override { return _last_rssi; }
  float getLastSNR() const override { return _last_snr; }

  bool isReceiving() override { return _cad_busy; }
  int getNoiseFloor() const override { return _noise_floor_rssi; }
  void setCadBusy(bool busy) { _cad_busy = busy; }
  bool getCadBusy() const { return _cad_busy; }
  void setNoiseFloor(int rssi) { _noise_floor_rssi = rssi; }

  void setParams(float freq, float bw, uint8_t sf, uint8_t cr);
  void setTxPower(int8_t dbm);
  // Retourne bool (pas void) : MyMesh::setRxBoostedGain() fait
  // `return radio_driver.setRxBoostedGainMode(enable);` depuis la maj MeshCore.
  bool setRxBoostedGainMode(bool) { return true; }
  uint32_t getPacketsRecv() const { return _n_recv; }
  uint32_t getPacketsRecvErrors() const { return _n_recv_errors; }
  uint32_t getPacketsSent() const { return _n_sent; }
  void resetStats() { _n_recv = _n_sent = _n_recv_errors = 0; }

  float getFreq() const { return _freq; }
  float getBw() const { return _bw; }
  uint8_t getSf() const { return _sf; }
  uint8_t getCr() const { return _cr; }
  int8_t getTxPowerDbm() const { return _tx_power_dbm; }

private:
  std::deque<RadioLogEntry>& _log;
  std::deque<RxQueueEntry>& _rxQueue;
  const char* _tag;
  bool _sending = false;
  unsigned long _send_done_at = 0;
  float _last_rssi = -90.0f;
  float _last_snr = 8.0f;
  uint32_t _n_recv = 0, _n_sent = 0, _n_recv_errors = 0;
  float _freq = 0, _bw = 0;
  uint8_t _sf = 0, _cr = 0;
  int8_t _tx_power_dbm = 0;
  bool _cad_busy = false;
  int _noise_floor_rssi = 0;
};

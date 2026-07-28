#pragma once

#include <Dispatcher.h>  // mesh::Radio
#include <deque>
#include <vector>
#include "SimWorld.h"

// ============================================================================
// SimRadio — implémentation de mesh::Radio (cf. lib/MeshCore/src/
// Dispatcher.h) qui ne touche à aucun matériel réel : TX est loggé dans
// SimWorld (visible CLI "sim status"/dashboard web), RX est consommé depuis
// la file d'injection correspondante, remplie par "sim rx mesh|aprs <hex>"
// (CLI, cf. native/sim/SimCli.h) ou POST /api/rx (web).
//
// Une seule instance pour la radio mesh (868 MHz) et une pour l'APRS
// (433 MHz), cf. variant_native/target.h — chacune pointe vers le log/la
// file de SimWorld qui lui correspond.
// ============================================================================

class SimRadio : public mesh::Radio {
public:
  SimRadio(std::deque<RadioLogEntry>& log, std::deque<std::vector<uint8_t>>& rxQueue, const char* tag)
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

  // --- Surface étendue de RadioLibWrapper (lib/MeshCore/src/helpers/
  // radiolib/RadioLibWrappers.h) — MyMesh.cpp appelle ces méthodes
  // directement sur le type concret `radio_driver`, pas via mesh::Radio.
  // Pas de vraie config radio possible en simulateur : no-op loggué.
  void setParams(float freq, float bw, uint8_t sf, uint8_t cr);
  void setTxPower(int8_t dbm);
  void setRxBoostedGainMode(bool) {}
  uint32_t getPacketsRecv() const { return _n_recv; }
  uint32_t getPacketsRecvErrors() const { return _n_recv_errors; }
  uint32_t getPacketsSent() const { return _n_sent; }
  void resetStats() { _n_recv = _n_sent = _n_recv_errors = 0; }

  // Derniers paramètres appliqués via setParams()/setTxPower() — utile pour
  // exposer l'état radio complet côté simulateur (aucun autre endroit ne les
  // garde, la "config radio" réelle vivant dans le registre SX1262).
  float getFreq() const { return _freq; }
  float getBw() const { return _bw; }
  uint8_t getSf() const { return _sf; }
  uint8_t getCr() const { return _cr; }
  int8_t getTxPowerDbm() const { return _tx_power_dbm; }

private:
  std::deque<RadioLogEntry>& _log;
  std::deque<std::vector<uint8_t>>& _rxQueue;
  const char* _tag;
  bool _sending = false;
  unsigned long _send_done_at = 0;
  float _last_rssi = -90.0f;
  float _last_snr = 8.0f;
  uint32_t _n_recv = 0, _n_sent = 0, _n_recv_errors = 0;
  float _freq = 0, _bw = 0;
  uint8_t _sf = 0, _cr = 0;
  int8_t _tx_power_dbm = 0;
};

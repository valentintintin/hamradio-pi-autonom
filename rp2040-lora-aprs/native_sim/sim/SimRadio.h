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

  // --- CAD matériel simulé / bruit de fond ----------------------------------
  // isReceiving() (utilisé par AprsDispatcher::isReceiving() côté APRS, et
  // par MyMesh côté mesh pour le carrier-sense avant TX) vaut toujours false
  // par défaut ici (pas de vraie détection radio en simulateur) — ce bouton
  // ("sim cad mesh|aprs on|off", cf. SimCli.h, ou le dashboard web) force un
  // "canal occupé" pour tester le comportement CSMA (report d'émission) sans
  // dépendre d'une vraie collision. getNoiseFloor() est de même figé à 0 par
  // défaut (mesh::Radio) ; réglable ici ("sim set noise mesh|aprs <rssi>")
  // pour que stats.noise_floor (cf. MyMesh.cpp) et le dashboard reflètent une
  // valeur de bruit de fond réaliste.
  bool isReceiving() override { return _cad_busy; }
  int getNoiseFloor() const override { return _noise_floor_rssi; }
  void setCadBusy(bool busy) { _cad_busy = busy; }
  bool getCadBusy() const { return _cad_busy; }
  void setNoiseFloor(int rssi) { _noise_floor_rssi = rssi; }

  // --- Surface étendue de RadioLibWrapper (lib/MeshCore/src/helpers/
  // radiolib/RadioLibWrappers.h) — MyMesh.cpp appelle ces méthodes
  // directement sur le type concret `radio_driver`, pas via mesh::Radio.
  // Pas de vraie config radio possible en simulateur : no-op loggué.
  void setParams(float freq, float bw, uint8_t sf, uint8_t cr);
  void setTxPower(int8_t dbm);
  // bool (pas void) depuis la maj MeshCore : MyMesh::setRxBoostedGain()
  // fait `return radio_driver.setRxBoostedGainMode(enable);`. Pas de vraie
  // config radio en simulateur : toujours "réussi".
  bool setRxBoostedGainMode(bool) { return true; }
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

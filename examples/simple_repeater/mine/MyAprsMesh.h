#ifndef MESHCORE_MYAPRSMESH_H
#define MESHCORE_MYAPRSMESH_H
#include "MyAprsDispatcher.h"

namespace mine {
class MyAprsMesh : public MyAprsDispatcher {
  mesh::RNG * _rng;

public:
  MyAprsMesh(mesh::MainBoard& board, mesh::Radio& radio, mesh::MillisecondClock& ms, mesh::RNG& rng,
    mesh::RTCClock& rtc);

  void handleCommand(uint32_t sender_timestamp, char* command, char* reply);

protected:
  mesh::DispatcherAction onRecvPacket(AprsPacket *pkt) override;

  void logRxRaw(float snr, float rssi, const uint8_t raw[], int len) override;
  void logRx(AprsPacket *packet, float score) override;
  void logTx(AprsPacket *packet) override;
  void logTxFail(AprsPacket *packet) override;
  const char *getLogDateTime() override;
  uint32_t getRetransmitDelay(const AprsPacket *packet) const;
};

}

#endif // MESHCORE_MYAPRSMESH_H

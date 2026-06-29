//
// Created by valentin on 22/06/2026.
//

#include "MyAprsMesh.h"

void mine::MyAprsMesh::handleCommand(uint32_t sender_timestamp, char *command, char *reply) {
  while (*command == ' ') command++; // skip leading spaces

  if (memcmp(command, "test", 4) == 0) {
    strcpy(reply, "OK");
  } else {
    // TODO appeler les autres
  }
}

mesh::DispatcherAction mine::MyAprsMesh::onRecvPacket(AprsPacket *pkt) {
  if (strcasecmp(pkt->packet.destination, APRS_MY_CALL) == 0) {
    // TODO c'est pour moi
    return ACTION_RELEASE;
  }

  if (aprs::canBeDigipeated(pkt->packet.path, sizeof(pkt->packet.path), APRS_MY_CALL)) { // TODO 30s window
    const auto d = getRetransmitDelay(pkt);
    return ACTION_RETRANSMIT_DELAYED(0, d); // Priorite toujours max
  }

  return ACTION_RELEASE;
}

void mine::MyAprsMesh::logRxRaw(float snr, float rssi, const uint8_t raw[], int len) {
  MyAprsDispatcher::logRxRaw(snr, rssi, raw, len);
}
void mine::MyAprsMesh::logRx(AprsPacket *packet, float score) {
  MyAprsDispatcher::logRx(packet, score);
}
void mine::MyAprsMesh::logTx(AprsPacket *packet) {
  MyAprsDispatcher::logTx(packet);
}
void mine::MyAprsMesh::logTxFail(AprsPacket *packet) {
  MyAprsDispatcher::logTxFail(packet);
}
const char *mine::MyAprsMesh::getLogDateTime() {
  return MyAprsDispatcher::getLogDateTime();
}
uint32_t mine::MyAprsMesh::getRetransmitDelay(const AprsPacket *packet) const {
  uint32_t t = (_radio->getEstAirtimeFor(packet->size)/* * _prefs.tx_delay_factor*/);
  return _rng->nextInt(0, 5*t + 1);
}
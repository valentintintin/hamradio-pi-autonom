#include "MeshcoreRepeater.h"

MeshcoreRepeater::MeshcoreRepeater(mesh::MainBoard &board, mesh::Radio &radio,
                                                       mesh::MillisecondClock &ms, mesh::RNG &rng,
                                                       mesh::RTCClock &rtc, mesh::MeshTables &tables)
    : MyMesh(board, radio, ms, rng, rtc, tables) {}

void MeshcoreRepeater::handleCommand(const uint32_t sender_timestamp, char *command, char *reply) {
  while (*command == ' ') {
    command++; // skip leading spaces
  }

  // TODO appéler notre cli

  MyMesh::handleCommand(sender_timestamp, command, reply);
}

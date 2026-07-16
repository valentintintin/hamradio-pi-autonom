#include "MeshcoreRepeater.h"
#include "cli/CommandHandler.h"
#include <cstring>

extern CommandHandler command_handler;

MeshcoreRepeater::MeshcoreRepeater(mesh::MainBoard& board, mesh::Radio& radio,
                                   mesh::MillisecondClock& ms, mesh::RNG& rng,
                                   mesh::RTCClock& rtc, mesh::MeshTables& tables)
    : MyMesh(board, radio, ms, rng, rtc, tables) {}

void MeshcoreRepeater::handleCommand(const uint32_t sender_timestamp, char* command, char* reply) {
  while (*command == ' ') {
    command++; // skip leading spaces
  }

  if (strcmp(command, "my ") == 0) {
    command_handler.execute(command + 3, reply, 160, sender_timestamp == 0);
    return;
  }

  return MyMesh::handleCommand(sender_timestamp, command, reply);
}

#pragma once

#include "MyMesh.h"

// ============================================================================
// MeshcoreRepeater — MyMesh (exemple MeshCore simple_repeater) + branchement
// de notre CLI custom (relay/aprs/version/clockdate...) en repli quand la CLI
// MeshCore standard ne reconnaît pas la commande. Voir handleCommand().
// ============================================================================
class MeshcoreRepeater : public MyMesh {
public:
  MeshcoreRepeater(mesh::MainBoard& board, mesh::Radio& radio, mesh::MillisecondClock& ms,
                   mesh::RNG& rng, mesh::RTCClock& rtc, mesh::MeshTables& tables);
  void handleCommand(uint32_t sender_timestamp, char* command, char* reply) override;

  const char* getFirmwareVer() override { return FIRMWARE_VERSION; }
  const char* getBuildDate() override { return FIRMWARE_BUILD_DATE; }
};

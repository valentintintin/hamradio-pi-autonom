#pragma once

#include "MyMesh.h"
#include "MeshcoreRepeater.h"

#define FIRMWARE_VERSION "v1.16.0 Bridge Meshtastic"
#define FIRMWARE_BUILD_DATE "14 Jun 2026"

// ReSharper disable once CppPolymorphicClassWithNonVirtualPublicDestructor
class MeshcoreRepeater : public MyMesh {
 public:
  MeshcoreRepeater(mesh::MainBoard &board, mesh::Radio &radio, mesh::MillisecondClock &ms,
                             mesh::RNG &rng, mesh::RTCClock &rtc, mesh::MeshTables &tables);
  void handleCommand(uint32_t sender_timestamp, char *command, char *reply) override;

  const char* getFirmwareVer() override { return FIRMWARE_VERSION; }
  const char* getBuildDate() override { return FIRMWARE_BUILD_DATE; }
};
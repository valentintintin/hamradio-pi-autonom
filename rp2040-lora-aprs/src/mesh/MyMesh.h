#pragma once

// ============================================================================
// MyMesh — Repeater MeshCore
//
// Sous-classe de mesh::Mesh conforme à l'API MeshCore upstream.
// Inspiré de MeshCore/examples/simple_repeater/MyMesh.h
// ============================================================================

#include <Arduino.h>
#include <Mesh.h>
#include <helpers/CommonCLI.h>
#include <helpers/IdentityStore.h>
#include <helpers/AdvertDataHelpers.h>
#include <helpers/ArduinoHelpers.h>
#include <helpers/StaticPoolPacketManager.h>
#include <helpers/SimpleMeshTables.h>
#include <target.h>

// Forward
class MeshAprsBridge;

#ifndef FIRMWARE_BUILD_DATE
  #define FIRMWARE_BUILD_DATE  __DATE__
#endif
#ifndef FIRMWARE_VERSION
  #define FIRMWARE_VERSION     "v0.1.0"
#endif
#define FIRMWARE_ROLE          "repeater"

#ifndef MAX_NEIGHBOURS
  #define MAX_NEIGHBOURS  50
#endif

struct NeighbourInfo {
  mesh::Identity id;
  uint32_t advert_timestamp;
  uint32_t heard_timestamp;
  int8_t snr;  // x4
};

class MyMesh : public mesh::Mesh, public CommonCLICallbacks {
public:
  MyMesh(mesh::MainBoard& board, mesh::Radio& radio, mesh::MillisecondClock& ms,
         mesh::RNG& rng, mesh::RTCClock& rtc, mesh::MeshTables& tables);

  void begin(FILESYSTEM* fs);
  void loop();

  // Bridge vers APRS (optionnel)
  void setBridge(MeshAprsBridge* bridge) { _bridge = bridge; }

  // CLI — appelé depuis serial et mesh
  void handleCommand(uint32_t sender_timestamp, char* command, char* reply);

  // --- CommonCLICallbacks -------------------------------------------------
  void savePrefs() override;
  const char* getFirmwareVer() override { return FIRMWARE_VERSION; }
  const char* getBuildDate() override { return FIRMWARE_BUILD_DATE; }
  const char* getRole() override { return FIRMWARE_ROLE; }
  bool formatFileSystem() override;
  void sendSelfAdvertisement(int delay_millis) override;
  void updateAdvertTimer() override;
  void updateFloodAdvertTimer() override;
  void setLoggingOn(bool enable) override { _logging = enable; }
  void eraseLogFile() override;
  void dumpLogFile() override;
  void setTxPower(uint8_t power_dbm) override;
  void formatNeighborsReply(char* reply) override;
  void removeNeighbor(const uint8_t* pubkey, int key_len) override;
  mesh::LocalIdentity& getSelfId() override { return self_id; }
  void saveIdentity(const mesh::LocalIdentity& new_id) override;
  void clearStats() override;
  void applyTempRadioParams(float freq, float bw, uint8_t sf, uint8_t cr, int timeout_mins) override;

  NodePrefs* getNodePrefs() { return &_prefs; }

protected:
  float getAirtimeBudgetFactor() const override { return _prefs.airtime_factor; }
  bool allowPacketForward(const mesh::Packet* packet) override;
  uint32_t getRetransmitDelay(const mesh::Packet* packet) override;
  uint32_t getDirectRetransmitDelay(const mesh::Packet* packet) override;

  // Callbacks MeshCore
  void onAdvertRecv(mesh::Packet* packet, const mesh::Identity& id,
                    uint32_t timestamp, const uint8_t* app_data, size_t app_data_len) override;

  void onPeerDataRecv(mesh::Packet* packet, uint8_t type, int sender_idx,
                      const uint8_t* secret, uint8_t* data, size_t len) override;

private:
  FILESYSTEM* _fs;
  CommonCLI _cli;
  NodePrefs _prefs;
  MeshAprsBridge* _bridge;
  bool _logging;

  // Timers adverts
  unsigned long _next_local_advert;
  unsigned long _next_flood_advert;

  // Radio params temporaires (via CommonCLI)
  unsigned long _set_radio_at;
  unsigned long _revert_radio_at;
  float _pending_freq, _pending_bw;
  uint8_t _pending_sf, _pending_cr;

  // Voisins
#if MAX_NEIGHBOURS
  NeighbourInfo _neighbours[MAX_NEIGHBOURS];
#endif

  mesh::Packet* createSelfAdvert();
  void putNeighbour(const mesh::Identity& id, uint32_t timestamp, float snr);
};

#pragma once

#include "ChannelDetails.h"
#include "MyMesh.h"
#include "config/Settings.h"

struct MeshcoreChannelRegion {
    ChannelDetails channel_details;
    char region[31] = {'*'};
};

// Secret/hash de chaque canal dérivé du nom par sha256, jamais stocké dans
// les settings eux-mêmes.
class MeshcoreRepeater : public MyMesh {
public:
  MeshcoreRepeater(mesh::MainBoard& board, mesh::Radio& radio, mesh::MillisecondClock& ms,
                   mesh::RNG& rng, mesh::RTCClock& rtc, mesh::MeshTables& tables);
  void handleCommand(uint32_t sender_timestamp, char* command, char* reply) /*override*/;

  const char* getFirmwareVer() override { return FIRMWARE_VERSION; }
  const char* getBuildDate() override { return FIRMWARE_BUILD_DATE; }

  void loadChannelsFromSettings(const Settings& settings);

protected:
    int searchChannelsByHash(const uint8_t *hash, mesh::GroupChannel dest[], int max_matches) override;
    void onGroupDataRecv(mesh::Packet *packet, uint8_t type, const mesh::GroupChannel &channel, uint8_t *data,
                       size_t len) override;
private:
    MeshcoreChannelRegion *addChannel(uint8_t index, const char *name, const char *region);
    bool getKeyFromRegion(const char *region_name, TransportKey &scope);

    MeshcoreChannelRegion channels[MAX_GROUP_CHANNELS] = {};
};

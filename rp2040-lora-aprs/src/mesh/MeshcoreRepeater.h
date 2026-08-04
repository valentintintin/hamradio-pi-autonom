#pragma once

#include "ChannelDetails.h"
#include "MyMesh.h"
#include "config/Settings.h"

struct MeshcoreChannelRegion {
    ChannelDetails channel_details;
    char region[31] = {'*'};
};

// ============================================================================
// MeshcoreRepeater — MyMesh (exemple MeshCore simple_repeater) + branchement
// de notre CLI custom (relay/aprs/version/clockdate...) en repli quand la CLI
// MeshCore standard ne reconnaît pas la commande. Voir handleCommand().
//
// Les canaux de groupe (nom + région, cf. config/Settings.h:
// MeshChannelSettings, réglables via "set mesh.channel.N.name/region") sont
// chargés dans `channels[]` par loadChannelsFromSettings(), à appeler après
// bootLoadConfig() (cf. core/Boot.cpp) — le secret/hash réel de chaque canal
// est dérivé du nom par sha256 (add_meshcore_bridge_channel), jamais stocké
// dans les settings eux-mêmes.
// ============================================================================
class MeshcoreRepeater : public MyMesh {
public:
  MeshcoreRepeater(mesh::MainBoard& board, mesh::Radio& radio, mesh::MillisecondClock& ms,
                   mesh::RNG& rng, mesh::RTCClock& rtc, mesh::MeshTables& tables);
  void handleCommand(uint32_t sender_timestamp, char* command, char* reply) /*override*/;

  const char* getFirmwareVer() override { return FIRMWARE_VERSION; }
  const char* getBuildDate() override { return FIRMWARE_BUILD_DATE; }

  // (Re)charge tous les slots de channels[] depuis settings.mesh_channels[]
  // (slots dont le nom est vide sont laissés/remis à zéro, donc ignorés par
  // searchChannelsByHash). Peut être rappelée après un "set mesh.channel...".
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

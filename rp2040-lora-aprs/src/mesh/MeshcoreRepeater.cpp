#include "MeshcoreRepeater.h"
#include "cli/CommandHandler.h"
#include <cstring>
#include <base64.hpp>

extern CommandHandler command_handler;

#define PUBLIC_GROUP_PSK                "izOH6cXN6mrJ5e26oRXNcg=="

MeshcoreRepeater::MeshcoreRepeater(mesh::MainBoard& board, mesh::Radio& radio,
                                   mesh::MillisecondClock& ms, mesh::RNG& rng,
                                   mesh::RTCClock& rtc, mesh::MeshTables& tables)
    : MyMesh(board, radio, ms, rng, rtc, tables) {}

void MeshcoreRepeater::handleCommand(const uint32_t sender_timestamp, char* command, char* reply) {
  while (*command == ' ') {
    command++;
  }

  if (strcmp(command, "my ") == 0) {
    command_handler.execute(command + 3, reply, CLI_RADIO_REPLY_MAX_LEN, sender_timestamp == 0);
    return;
  }

  return MyMesh::handleCommand(sender_timestamp, command, reply);
}

int MeshcoreRepeater::searchChannelsByHash(const uint8_t* hash, mesh::GroupChannel dest[], int max_matches) {
  int n = 0;
  for (int i = 0; i < MAX_GROUP_CHANNELS && n < max_matches; i++) {
    // name vide = slot inutilisé, à ignorer même si hash[0]==0 par coïncidence
    // (PATH_HASH_SIZE == 1, donc 1 chance sur 256 de collision).
    if (channels[i].channel_details.name[0] == '\0') {
      continue;
    }
    if (channels[i].channel_details.channel.hash[0] == hash[0]) {
      dest[n++] = channels[i].channel_details.channel;
    }
  }
  return n;
}

void MeshcoreRepeater::loadChannelsFromSettings(const Settings& settings) {
  for (int i = 0; i < MAX_GROUP_CHANNELS; i++) {
    const MeshChannelSettings& cfg = settings.mesh_channels[i];
    if (cfg.name[0] == '\0') {
      channels[i] = MeshcoreChannelRegion{};
      continue;
    }
    addChannel(i, cfg.name, cfg.region);
  }
}

void MeshcoreRepeater::onGroupDataRecv(mesh::Packet *packet, uint8_t type,
                                                 const mesh::GroupChannel &channel, uint8_t *data,
                                                 size_t len) {
  const uint8_t txt_type = data[4];
  if (type == PAYLOAD_TYPE_GRP_TXT && len > 5 && (txt_type >> 2) == 0) {
    uint32_t timestamp;
    memcpy(&timestamp, data, 4);

    // len peut dépasser la longueur d'origine (padding zéro) : on retermine
    // la chaîne nous-mêmes.
    data[len] = 0;
    const auto text = (const char *)&data[5];

    LOG_D("MESH", "Received TXT '%s' for channel %d", text, channel);
  }
}

MeshcoreChannelRegion *MeshcoreRepeater::addChannel(const uint8_t index,
  const char *name, const char *region) {
  if (index >= MAX_GROUP_CHANNELS) {
    LOG_W("MESH", "Invalid channel index %d (max: %d)", index, MAX_GROUP_CHANNELS - 1);
    return nullptr;
  }

  LOG_D("MESH", "Configure channel '%s' at index %d", name, index);

  if (strlen(name) >= sizeof(channels[index].channel_details.name)) {
    LOG_W("MESH", "Invalid channel name '%s': length %d >= %d", name, strlen(name),
                       sizeof(channels[index].channel_details.name));
    return nullptr;
  }

  if (strlen(region) >= sizeof(channels[index].region)) {
    LOG_W("MESH", "Invalid region '%s' for channel '%s': length %d >= %d", region, name,
                       strlen(region), sizeof(channels[index].region));
    return nullptr;
  }

  const auto bridge_channel = &channels[index];
  memset(bridge_channel->channel_details.channel.secret, 0,
         sizeof(bridge_channel->channel_details.channel.secret));
  memset(bridge_channel->channel_details.channel.hash, 0,
         sizeof(bridge_channel->channel_details.channel.hash));
  memset(bridge_channel->channel_details.name, 0, sizeof(bridge_channel->channel_details.name));
  memset(&bridge_channel->region, 0, sizeof(bridge_channel->region));

  int len = 0;

  if (strcasecmp(name, "Public") == 0) {
    const auto psk_base64 = PUBLIC_GROUP_PSK;
    len = decode_base64((unsigned char *)psk_base64, strlen(psk_base64),
                        bridge_channel->channel_details.channel.secret);
    if (len == 32 || len == 16) {
      mesh::Utils::sha256(bridge_channel->channel_details.channel.hash,
                          sizeof(bridge_channel->channel_details.channel.hash),
                          bridge_channel->channel_details.channel.secret, len);
    } else {
      LOG_W("MESH", "Invalid decoded PSK length for public channel: %d", len);

      return nullptr;
    }
  } else {
    len = 16;
    mesh::Utils::sha256(bridge_channel->channel_details.channel.secret, len, (const uint8_t *)name,
                        strlen(name));
  }

  mesh::Utils::sha256(bridge_channel->channel_details.channel.hash,
                      sizeof(bridge_channel->channel_details.channel.hash),
                      bridge_channel->channel_details.channel.secret, len);

  StrHelper::strncpy(bridge_channel->channel_details.name, name,
                     sizeof(bridge_channel->channel_details.name));

  if (!strlen(region)) {
    StrHelper::strncpy(bridge_channel->region, "*", sizeof(bridge_channel->region));
  } else {
    StrHelper::strncpy(bridge_channel->region, region, sizeof(bridge_channel->region));
  }

  LOG_I("MESH", "Channel '%s' [%s] configured (secret[0]=0x%x hash[0]=0x%x) at index %d",
                     name, bridge_channel->region, bridge_channel->channel_details.channel.secret[0],
                     bridge_channel->channel_details.channel.hash[0], index);

  return bridge_channel;
}

bool MeshcoreRepeater::getKeyFromRegion(const char *region_name, TransportKey &scope) {
  memset(scope.key, 0, sizeof(scope.key));
  if (strcmp(region_name, "*") == 0) {
    return false; // use fallback
  }
  if (region_name[0] == '$') {
    return false; // private region unsupported without keystore
  }
  char region_tag[32] = {};
  if (region_name[0] == '#') {
    StrHelper::strncpy(region_tag, region_name, sizeof(region_tag));
  } else {
    region_tag[0] = '#';
    StrHelper::strncpy(&region_tag[1], region_name, sizeof(region_tag) - 1);
  }
  mesh::Utils::sha256(scope.key, sizeof(scope.key), (const uint8_t *)region_tag, strlen(region_tag));
  return true;
}
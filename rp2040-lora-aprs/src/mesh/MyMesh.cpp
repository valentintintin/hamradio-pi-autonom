#include "MyMesh.h"
#include "../bridge/MeshAprsBridge.h"
#include "config/Log.h"

#define TAG "MESH"

// ============================================================================
// Constructeur — conforme à l'API MeshCore upstream
// ============================================================================
MyMesh::MyMesh(mesh::MainBoard& board, mesh::Radio& radio, mesh::MillisecondClock& ms,
               mesh::RNG& rng, mesh::RTCClock& rtc, mesh::MeshTables& tables)
  : mesh::Mesh(radio, ms, rng, rtc, *new StaticPoolPacketManager(32), tables),
    _key_store(), _region_map(_key_store),
    _cli(board, rtc, sensors, _region_map, _acl, &_prefs, this),
    _fs(nullptr), _bridge(nullptr), _logging(false),
    _next_local_advert(0), _next_flood_advert(0),
    _set_radio_at(0), _revert_radio_at(0)
{
#if MAX_NEIGHBOURS
  memset(_neighbours, 0, sizeof(_neighbours));
#endif

  // Défauts NodePrefs
  memset(&_prefs, 0, sizeof(_prefs));
  _prefs.airtime_factor = 1.0f;
  _prefs.rx_delay_base = 0.0f;
  _prefs.tx_delay_factor = 0.5f;
  _prefs.direct_tx_delay_factor = 0.5f;
  StrHelper::strncpy(_prefs.node_name, ADVERT_NAME, sizeof(_prefs.node_name));
  _prefs.node_lat = ADVERT_LAT;
  _prefs.node_lon = ADVERT_LON;
  StrHelper::strncpy(_prefs.password, ADMIN_PASSWORD, sizeof(_prefs.password));
  _prefs.freq = LORA_FREQ;
  _prefs.bw = LORA_BW;
  _prefs.sf = LORA_SF;
  _prefs.cr = LORA_CR;
  _prefs.tx_power_dbm = LORA_TX_POWER;
  _prefs.advert_interval = 1;         // 2 minutes
  _prefs.flood_advert_interval = 12;  // 12 heures
  _prefs.flood_max = 64;
}

// ============================================================================
// begin — charge prefs, applique radio, démarre les timers
// ============================================================================
void MyMesh::begin(FILESYSTEM* fs) {
  _fs = fs;
  mesh::Mesh::begin();

  _cli.loadPrefs(_fs);

  // Appliquer les params radio depuis les prefs
  radio_set_params(_prefs.freq, _prefs.bw, _prefs.sf, _prefs.cr);
  radio_set_tx_power(_prefs.tx_power_dbm);

  updateAdvertTimer();
  updateFloodAdvertTimer();

  LOG_I(TAG, "MeshCore démarré: %s", _prefs.node_name);
}

// ============================================================================
// loop — adverts périodiques + radio params temporaires
// ============================================================================
void MyMesh::loop() {
  mesh::Mesh::loop();

  // Flood advert (prioritaire sur local)
  if (_next_flood_advert && millisHasNowPassed(_next_flood_advert)) {
    mesh::Packet* pkt = createSelfAdvert();
    if (pkt) sendFlood(pkt);
    updateFloodAdvertTimer();
    updateAdvertTimer();
  } else if (_next_local_advert && millisHasNowPassed(_next_local_advert)) {
    mesh::Packet* pkt = createSelfAdvert();
    if (pkt) sendZeroHop(pkt);
    updateAdvertTimer();
  }

  // Application de params radio temporaires (CommonCLI)
  if (_set_radio_at && millisHasNowPassed(_set_radio_at)) {
    _set_radio_at = 0;
    radio_set_params(_pending_freq, _pending_bw, _pending_sf, _pending_cr);
    LOG_D(TAG, "Radio params temporaires appliqués");
  }
  if (_revert_radio_at && millisHasNowPassed(_revert_radio_at)) {
    _revert_radio_at = 0;
    radio_set_params(_prefs.freq, _prefs.bw, _prefs.sf, _prefs.cr);
    LOG_D(TAG, "Radio params restaurés");
  }
}

// ============================================================================
// Advert — création du paquet d'annonce
// ============================================================================
mesh::Packet* MyMesh::createSelfAdvert() {
  uint8_t app_data[MAX_ADVERT_DATA_SIZE];
  uint8_t app_data_len;
  {
    AdvertDataBuilder builder(ADV_TYPE_REPEATER, _prefs.node_name, _prefs.node_lat, _prefs.node_lon);
    app_data_len = builder.encodeTo(app_data);
  }
  return createAdvert(self_id, app_data, app_data_len);
}

// ============================================================================
// onAdvertRecv — voisins + notification bridge
// ============================================================================
void MyMesh::onAdvertRecv(mesh::Packet* packet, const mesh::Identity& id,
                          uint32_t timestamp, const uint8_t* app_data, size_t app_data_len) {
  mesh::Mesh::onAdvertRecv(packet, id, timestamp, app_data, app_data_len);

  // Tracker les voisins directs (zero-hop)
  if (packet->getPathHashCount() == 0) {
    AdvertDataParser parser(app_data, app_data_len);
    if (parser.isValid() && parser.getType() == ADV_TYPE_REPEATER) {
      putNeighbour(id, timestamp, packet->getSNR());
    }
  }

  LOG_D(TAG, "Advert reçu %02X%02X... hops=%d", id.pub_key[0], id.pub_key[1], packet->getPathHashCount());

  if (_bridge) {
    _bridge->onMeshAdvertReceived(id, app_data, app_data_len);
  }
}

void MyMesh::onPeerDataRecv(mesh::Packet* packet, uint8_t type, int sender_idx,
                            const uint8_t* secret, uint8_t* data, size_t len) {
  // Repeater minimal : pas de gestion de clients pour l'instant
  LOG_D(TAG, "PeerData type=%d len=%d", type, (int)len);
}

// ============================================================================
// Voisins
// ============================================================================
void MyMesh::putNeighbour(const mesh::Identity& id, uint32_t timestamp, float snr) {
#if MAX_NEIGHBOURS
  uint32_t oldest_ts = 0xFFFFFFFF;
  NeighbourInfo* slot = &_neighbours[0];

  for (int i = 0; i < MAX_NEIGHBOURS; i++) {
    if (id.matches(_neighbours[i].id)) { slot = &_neighbours[i]; break; }
    if (_neighbours[i].heard_timestamp < oldest_ts) {
      slot = &_neighbours[i];
      oldest_ts = _neighbours[i].heard_timestamp;
    }
  }

  slot->id = id;
  slot->advert_timestamp = timestamp;
  slot->heard_timestamp = getRTCClock()->getCurrentTime();
  slot->snr = (int8_t)(snr * 4);
#endif
}

// ============================================================================
// Forwarding
// ============================================================================
bool MyMesh::allowPacketForward(const mesh::Packet* packet) {
  if (_prefs.disable_fwd) return false;
  if (packet->isRouteFlood() && packet->getPathHashCount() >= _prefs.flood_max) return false;
  return true;
}

uint32_t MyMesh::getRetransmitDelay(const mesh::Packet* packet) {
  uint32_t t = _radio->getEstAirtimeFor(packet->getPathByteLen() + packet->payload_len + 2) * _prefs.tx_delay_factor;
  return getRNG()->nextInt(0, 5 * t + 1);
}

uint32_t MyMesh::getDirectRetransmitDelay(const mesh::Packet* packet) {
  uint32_t t = _radio->getEstAirtimeFor(packet->getPathByteLen() + packet->payload_len + 2) * _prefs.direct_tx_delay_factor;
  return getRNG()->nextInt(0, 5 * t + 1);
}

// ============================================================================
// CLI — dispatch vers CommonCLI
// ============================================================================
void MyMesh::handleCommand(uint32_t sender_timestamp, char* command, char* reply) {
  while (*command == ' ') command++;
  _cli.handleCommand(sender_timestamp, command, reply);
}

// ============================================================================
// CommonCLICallbacks
// ============================================================================
void MyMesh::savePrefs() {
  _cli.savePrefs(_fs);
}

void MyMesh::sendSelfAdvertisement(int delay_millis, bool flood) {
  mesh::Packet* pkt = createSelfAdvert();
  if (pkt) {
    if (flood) {
      sendFlood(pkt, delay_millis);
    } else {
      sendZeroHop(pkt, delay_millis);
    }
    LOG_I(TAG, "Advert envoyé (%s, délai %dms)", flood ? "flood" : "local", delay_millis);
  } else {
    LOG_E(TAG, "Impossible de créer l'advert");
  }
}

void MyMesh::updateAdvertTimer() {
  if (_prefs.advert_interval > 0) {
    _next_local_advert = futureMillis(((uint32_t)_prefs.advert_interval) * 2 * 60 * 1000);
  } else {
    _next_local_advert = 0;
  }
}

void MyMesh::updateFloodAdvertTimer() {
  if (_prefs.flood_advert_interval > 0) {
    _next_flood_advert = futureMillis(((uint32_t)_prefs.flood_advert_interval) * 60 * 60 * 1000);
  } else {
    _next_flood_advert = 0;
  }
}

bool MyMesh::formatFileSystem() {
  return LittleFS.format();
}

void MyMesh::eraseLogFile() {
  if (_fs) _fs->remove("/packet_log");
}

void MyMesh::dumpLogFile() {
  File f = _fs->open("/packet_log", "r");
  if (f) {
    while (f.available()) {
      int c = f.read();
      if (c < 0) break;
      Serial.print((char)c);
    }
    f.close();
  }
}

void MyMesh::setTxPower(int8_t power_dbm) {
  radio_set_tx_power(power_dbm);
}

void MyMesh::applyTempRadioParams(float freq, float bw, uint8_t sf, uint8_t cr, int timeout_mins) {
  _set_radio_at = futureMillis(2000);
  _pending_freq = freq;
  _pending_bw = bw;
  _pending_sf = sf;
  _pending_cr = cr;
  _revert_radio_at = futureMillis(2000 + timeout_mins * 60 * 1000);
}

void MyMesh::saveIdentity(const mesh::LocalIdentity& new_id) {
  self_id = new_id;
  IdentityStore store(*_fs, "/identity");
  store.save("_main", self_id);
}

void MyMesh::clearStats() {
  radio_driver.resetStats();
  resetStats();
  ((SimpleMeshTables*)getTables())->resetStats();
}

void MyMesh::formatNeighborsReply(char* reply) {
  char* dp = reply;
#if MAX_NEIGHBOURS
  for (int i = 0; i < MAX_NEIGHBOURS && dp - reply < 134; i++) {
    if (_neighbours[i].heard_timestamp == 0) continue;
    if (dp != reply) *dp++ = '\n';
    char hex[10];
    mesh::Utils::toHex(hex, _neighbours[i].id.pub_key, 4);
    uint32_t secs_ago = getRTCClock()->getCurrentTime() - _neighbours[i].heard_timestamp;
    sprintf(dp, "%s:%lu:%d", hex, (unsigned long)secs_ago, _neighbours[i].snr);
    while (*dp) dp++;
  }
#endif
  if (dp == reply) { strcpy(dp, "-none-"); dp += 6; }
  *dp = 0;
}

void MyMesh::removeNeighbor(const uint8_t* pubkey, int key_len) {
#if MAX_NEIGHBOURS
  for (int i = 0; i < MAX_NEIGHBOURS; i++) {
    if (memcmp(_neighbours[i].id.pub_key, pubkey, key_len) == 0) {
      _neighbours[i] = NeighbourInfo();
    }
  }
#endif
}

// ============================================================================
// Stats — formatage JSON pour CommonCLI
// ============================================================================
void MyMesh::formatStatsReply(char* reply) {
  StatsFormatHelper::formatCoreStats(reply, board, *_ms, _err_flags, _mgr);
}

void MyMesh::formatRadioStatsReply(char* reply) {
  StatsFormatHelper::formatRadioStats(reply, _radio, radio_driver, getTotalAirTime(), getReceiveAirTime());
}

void MyMesh::formatPacketStatsReply(char* reply) {
  StatsFormatHelper::formatPacketStats(reply, radio_driver, getNumSentFlood(), getNumSentDirect(),
                                       getNumRecvFlood(), getNumRecvDirect());
}

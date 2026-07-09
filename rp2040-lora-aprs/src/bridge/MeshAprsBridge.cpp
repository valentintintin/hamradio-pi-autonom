#include "MeshAprsBridge.h"
#include "config/Log.h"
#include <helpers/AdvertDataHelpers.h>
#include <string.h>

#define TAG "BRIDGE"

// Intervalle de broadcast APRS items (noeuds mesh)
#define ITEM_BROADCAST_INTERVAL (10 * 60 * 1000)  // 10 min
// Timeout pour considérer un noeud comme "disparu"
#define NODE_TIMEOUT            (60 * 60 * 1000)   // 1h

MeshAprsBridge::MeshAprsBridge(AprsEngine& aprs_engine)
  : _aprs(&aprs_engine), _next_item_broadcast(0)
{
  memset(_nodes, 0, sizeof(_nodes));
}

void MeshAprsBridge::loop() {
  // Broadcaster les noeuds mesh comme APRS items périodiquement
  // TODO: activer quand la logique sera finalisée
  /*
  if (_next_item_broadcast > 0 && millis() > _next_item_broadcast) {
    broadcastMeshNodesAsAprsItems();
    _next_item_broadcast = millis() + ITEM_BROADCAST_INTERVAL;
  }
  */
}

// ============================================================================
// Advert mesh reçu → tracker le noeud
// ============================================================================
void MeshAprsBridge::onMeshAdvertReceived(const mesh::Identity& id,
                                          const uint8_t* advert_data, size_t data_len) {
  TrackedMeshNode* node = findOrAllocNode(id.pub_key);
  if (!node) return;

  node->used = true;
  node->last_seen = millis();
  memcpy(node->pub_key_prefix, id.pub_key, 4);

  // Parser l'advert data via AdvertDataHelpers
  AdvertDataParser parser(advert_data, data_len);
  if (parser.isValid()) {
    const char* name = parser.getName();
    if (name && name[0]) {
      strncpy(node->name, name, sizeof(node->name) - 1);
      node->name[sizeof(node->name) - 1] = '\0';
    } else {
      snprintf(node->name, sizeof(node->name), "MC-%02X%02X",
               id.pub_key[0], id.pub_key[1]);
    }
    node->latitude = parser.getLat();
    node->longitude = parser.getLon();
  } else {
    snprintf(node->name, sizeof(node->name), "MC-%02X%02X",
             id.pub_key[0], id.pub_key[1]);
  }
  LOG_D(TAG, "Noeud mesh tracké: %s", node->name);

  // Activer le broadcast items si pas encore fait
  if (_next_item_broadcast == 0) {
    _next_item_broadcast = millis() + 30000; // 30s après le premier noeud vu
  }
}

void MeshAprsBridge::onAprsMessageForBridge(const char* from, const char* message) {
  // TODO: implémenter la passerelle message APRS → mesh
  // Ex: "!mesh <destination> <message>" → inject dans le mesh
}

// ============================================================================
// Envoyer les noeuds mesh comme APRS items
// ============================================================================
void MeshAprsBridge::broadcastMeshNodesAsAprsItems() {
  unsigned long now = millis();

  int broadcast_count = 0;
  for (int i = 0; i < MAX_TRACKED_NODES; i++) {
    if (!_nodes[i].used) continue;

    // Vérifier le timeout
    bool alive = (now - _nodes[i].last_seen) < NODE_TIMEOUT;

    // Envoyer comme APRS item seulement si on a une position
    if (_nodes[i].latitude != 0.0 || _nodes[i].longitude != 0.0) {
      _aprs->sendItem(
        _nodes[i].name,
        '#',    // symbole digipeater
        '/',    // table primaire
        "MeshCore node",
        _nodes[i].latitude,
        _nodes[i].longitude,
        0,      // altitude
        alive
      );
      broadcast_count++;
    }

    // Libérer les noeuds morts
    if (!alive) {
      LOG_D(TAG, "Noeud expiré: %s", _nodes[i].name);
      _nodes[i].used = false;
    }
  }
  LOG_T(TAG, "Broadcast %d items APRS", broadcast_count);
}

// ============================================================================
// Pool de noeuds
// ============================================================================
TrackedMeshNode* MeshAprsBridge::findOrAllocNode(const uint8_t* pub_key) {
  // Chercher un noeud existant
  for (int i = 0; i < MAX_TRACKED_NODES; i++) {
    if (_nodes[i].used && memcmp(_nodes[i].pub_key_prefix, pub_key, 4) == 0) {
      return &_nodes[i];
    }
  }

  // Allouer un nouveau slot
  for (int i = 0; i < MAX_TRACKED_NODES; i++) {
    if (!_nodes[i].used) return &_nodes[i];
  }

  // Pool plein — remplacer le plus ancien
  TrackedMeshNode* oldest = &_nodes[0];
  for (int i = 1; i < MAX_TRACKED_NODES; i++) {
    if (_nodes[i].last_seen < oldest->last_seen) {
      oldest = &_nodes[i];
    }
  }
  return oldest;
}

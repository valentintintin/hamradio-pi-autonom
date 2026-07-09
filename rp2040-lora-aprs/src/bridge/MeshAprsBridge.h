#pragma once

// ============================================================================
// MeshAprsBridge — Passerelle entre MeshCore (868) et APRS (433)
//
// Relaie les informations entre les deux stacks :
// - Noeuds mesh vus → APRS items/objects
// - Messages APRS → injection mesh (optionnel)
// - Position mesh → beacon APRS (optionnel)
// ============================================================================

#include <Identity.h>
#include "../aprs/AprsEngine.h"
#include <stdint.h>

// Nombre max de noeuds mesh trackés pour APRS
#define MAX_TRACKED_NODES 20

struct TrackedMeshNode {
  bool used;
  uint8_t pub_key_prefix[4];   // premiers bytes de la clé publique (ID)
  char name[16];               // nom du noeud (extrait de l'advert)
  unsigned long last_seen;     // millis() du dernier advert reçu
  float latitude;
  float longitude;
};

class MeshAprsBridge {
public:
  MeshAprsBridge(AprsEngine& aprs_engine);

  void loop();

  // Appelé par MyMesh quand un advert est reçu
  void onMeshAdvertReceived(const mesh::Identity& id, const uint8_t* advert_data, size_t data_len);

  // Appelé par AprsEngine quand un message APRS arrive (optionnel)
  void onAprsMessageForBridge(const char* from, const char* message);

private:
  AprsEngine* _aprs;
  TrackedMeshNode _nodes[MAX_TRACKED_NODES];

  // Timer pour envoyer les APRS items des noeuds mesh
  unsigned long _next_item_broadcast;

  TrackedMeshNode* findOrAllocNode(const uint8_t* pub_key);
  void broadcastMeshNodesAsAprsItems();
};

#include "MeshcoreRepeater.h"
#include "config/CommandHandler.h"
#include <string.h>

extern CommandHandler command_handler;

MeshcoreRepeater::MeshcoreRepeater(mesh::MainBoard& board, mesh::Radio& radio,
                                   mesh::MillisecondClock& ms, mesh::RNG& rng,
                                   mesh::RTCClock& rtc, mesh::MeshTables& tables)
    : MyMesh(board, radio, ms, rng, rtc, tables) {}

// ============================================================================
// La CLI MeshCore ("get "/"set ") ne signale pas une clé inconnue avec un
// texte homogène : "Unknown command" pour un verbe non reconnu, mais
// "unknown config: <clé>" (set) ou "??: <clé>" (get) pour une clé absente de
// son propre vocabulaire. Il faut reconnaître les trois pour retomber sur
// notre CommandHandler (aprs.*, relay.*, weather.*, energy.*, system.*).
//
// Limite connue : nos clés "radio.aprs.*"/"radio.mesh.freq" commencent par
// "radio", qui est aussi un préfixe reconnu par la CLI MeshCore (memcmp sur
// 5 caractères) — ces clés précises restent donc accessibles en direct
// (série sans préfixe "mesh ", ou message APRS) mais pas via ce chemin mesh.
// ============================================================================
static bool isUnrecognizedByMeshCli(const char* reply) {
  return strcmp(reply, "Unknown command") == 0 ||
         strncmp(reply, "unknown config: ", 16) == 0 ||
         strncmp(reply, "??: ", 4) == 0;
}

void MeshcoreRepeater::handleCommand(const uint32_t sender_timestamp, char* command, char* reply) {
  while (*command == ' ') {
    command++; // skip leading spaces
  }

  MyMesh::handleCommand(sender_timestamp, command, reply);

  if (isUnrecognizedByMeshCli(reply)) {
    // Garder une copie du message MeshCore d'origine : StringPrint (dans
    // CommandHandler::execute) vide `reply` dès sa construction, y compris
    // quand notre propre CLI ne reconnaît pas non plus la commande — sans
    // cette copie, un double-miss se traduirait par une réponse vide (donc
    // aucun paquet de réponse envoyé) au lieu du message d'erreur d'origine.
    char mesh_reply[160];
    strncpy(mesh_reply, reply, sizeof(mesh_reply) - 1);
    mesh_reply[sizeof(mesh_reply) - 1] = '\0';

    // Commandes custom (relay, aprs, version, clockdate, dfu...) non gérées
    // par la CLI MeshCore. sender_timestamp == 0 signale une origine locale
    // (série, "mesh <cmd>") plutôt qu'un message reçu sur le réseau mesh —
    // même convention que MeshCore lui-même (cf CommonCLI: "log"/"stats-*").
    if (!command_handler.execute(command, reply, sizeof(mesh_reply), sender_timestamp == 0)) {
      strncpy(reply, mesh_reply, sizeof(mesh_reply) - 1);
      reply[sizeof(mesh_reply) - 1] = '\0';
    }
  }
}

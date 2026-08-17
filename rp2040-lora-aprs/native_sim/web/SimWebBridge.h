#pragma once

// ============================================================================
// SimWebBridge — pont fichiers entre le simulateur et le serveur web Python
// (native_sim/web/server.py, qui sert l'interface et fait tourner un vrai
// serveur HTTP — cf. native_sim/web/static/). Pas de serveur HTTP ici :
//   - écrit périodiquement un instantané JSON complet de l'état simulé
//     (SimWorld + TelemetryData + réglages radio) dans
//     SIM_DATA_DIR/web/state.json (écriture atomique, temp + rename) ;
//   - consomme les fichiers de commande déposés par server.py dans
//     SIM_DATA_DIR/web/commands/ — chacun contient une ligne de commande au
//     format CLI ("sim rx aprs ascii ...", "relay 2 on"...),
//     passée telle quelle à simHandleCommand()/CommandHandler::execute()
//     (même pipeline que tasks/task_cli.cpp) puis supprimée.
//
// Démarrée une fois depuis core/Boot.cpp (bootInitCore(), #ifdef
// NATIVE_BUILD).
// ============================================================================
void startSimWebBridge();

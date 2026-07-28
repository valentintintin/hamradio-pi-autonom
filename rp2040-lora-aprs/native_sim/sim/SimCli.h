#pragma once

#include <Arduino.h>

// ============================================================================
// SimCli — commandes CLI propres au simulateur ("sim ..."), interceptées
// dans tasks/task_cli.cpp (#ifdef NATIVE_BUILD) avant l'appel à
// CommandHandler::execute() — celui-ci reste inchangé, ces commandes-là
// n'existent que côté natif.
//
// Commandes :
//   sim status                              — dump de l'état simulé (SimWorld)
//   sim set battery <mV> [<mA>]              — INA3221 canal 0 + MPPT/Victron
//   sim set solar <mV> [<mA>]                — INA3221 canal 2 + MPPT/Victron
//   sim set board5v <mV> [<mA>]              — INA3221 canal 1 (rail 5V board)
//   sim set weather <tempC> <humPct> <hPa>
//   sim rx mesh|aprs|fsk <hex>                — injecte une trame reçue (hex)
//   sim rx mesh|aprs|fsk ascii <texte>        — idem, payload texte brut (utile
//                                                pour l'APRS, protocole ASCII)
//   (fsk : 27 octets WH65B, consommés par le prochain cycle météo de
//   tasks/task_weather.cpp)
//
// Retourne true si `cmd` a été reconnue (et traitée), false sinon — même
// convention que CommandHandler::execute().
// ============================================================================
bool simHandleCommand(const char* cmd, Print& out);

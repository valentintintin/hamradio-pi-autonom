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
//   sim rx mesh|aprs|fsk [rssi <dBm>] [snr <dB>] <hex>
//                                              — injecte une trame reçue (hex),
//                                                RSSI/SNR simulés optionnels
//                                                (défaut -90dBm/8dB, -55dBm
//                                                pour fsk si rssi omis)
//   sim rx mesh|aprs|fsk [rssi <dBm>] [snr <dB>] ascii <texte>
//                                              — idem, payload texte brut (utile
//                                                pour l'APRS, protocole ASCII)
//   (fsk : 27 octets WH65B, consommés par le prochain cycle météo de
//   tasks/task_weather.cpp ; pas de SNR pour le FSK)
//   sim cad mesh|aprs on|off                  — force isReceiving() (canal
//                                                occupé/libre), simule un CAD
//                                                matériel en cours (cf.
//                                                radio.aprs.cad côté vrai HW)
//   sim set noise mesh|aprs <rssi_dBm>        — bruit de fond simulé
//                                                (getNoiseFloor())
//
// Retourne true si `cmd` a été reconnue (et traitée), false sinon — même
// convention que CommandHandler::execute().
// ============================================================================
bool simHandleCommand(const char* cmd, Print& out);

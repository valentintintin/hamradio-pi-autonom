#pragma once

#include <stdint.h>

// ============================================================================
// AprsRadioMode — bascule le SX1262 APRS (433 MHz) entre LoRa et FSK
//
// Le SX1262 dédié à l'APRS est aussi réutilisé en réception FSK pour décoder
// la station météo WH65B (FineOffset), qui n'émet pas en LoRa. Toute la
// configuration radio spécifique à ce basculement (fréquence/bitrate FSK,
// paramètres LoRa-APRS de retour) vit ici plutôt que dans la tâche météo,
// qui n'a pas à connaître les détails de configuration du radio APRS.
// ============================================================================

// Longueur fixe d'une trame WH65B (utilisée par le mode FSK ici, et par le
// décodeur FineOffsetWH65B côté tâche météo).
#define WH65B_PAYLOAD_LEN 27

typedef void (*AprsRadioRxCallback)();

namespace LoRa433RadioMode {

// Configure le SX1262 APRS en réception FSK (WH65B). `onRxDone` est appelé
// (depuis un contexte interruption RadioLib) quand une trame est reçue.
//
// Réutilisée telle quelle par l'émission CW/SSTV (aprs/SstvTransmitter.cpp) :
// RadioLib exige le modem en paquet GFSK pour transmitDirect()/directMode()
// (cf. SX126x::directMode(), qui refuse le mode direct hors GFSK) — cette
// config WH65B, déjà validée en réception réelle, suffit à satisfier cette
// exigence. La fréquence ici (WH65B_FREQ) n'a pas besoin de correspondre à la
// fréquence CW/SSTV réellement utilisée : MorseClient/SSTVClient retendent la
// porteuse eux-mêmes à chaque symbole (transmitDirect() par dot/dash/tone).
// Seule la puissance TX doit être réappliquée explicitement après l'appel
// (cf. SstvTransmitter::transmit()), le bitrate/déviation/bande/sync word
// WH65B étant sans effet sur une porteuse continue en mode direct.
bool switchToFsk(AprsRadioRxCallback onRxDone);

// Restaure la configuration LoRa-APRS normale.
bool switchToLora();

}  // namespace AprsRadioMode

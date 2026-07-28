#pragma once

#include <FreeRTOS.h>
#include <task.h>

// ============================================================================
// Réveil des tâches radio (APRS/mesh) depuis le contexte "interruption" DIO1
// du SX1262 — cf. lib/MeshCore/src/helpers/radiolib/RadioLibWrappers.cpp,
// fonction setFlag(), à laquelle un appel à notifyRadioActivityFromISR() a
// été ajouté (seule modification apportée à ce fichier vendoré — cf.
// commentaire sur place).
//
// `state` dans RadioLibWrappers.cpp est UN SEUL flag fichier-statique partagé
// par toutes les instances RadioLibWrapper — ce projet en a deux (radio mesh
// 868 MHz et radio APRS 433 MHz), toutes les deux passent par la même
// setFlag(). Comme cette fonction ne reçoit aucun contexte permettant de
// savoir laquelle des deux radios a réellement déclenché l'IRQ, on réveille
// les DEUX tâches à chaque appel : un réveil superflu est sans conséquence
// (la tâche revérifie son propre état et se rendort aussitôt si, de fait,
// rien ne la concernait) — rater un réveil, en revanche, serait un vrai bug.
//
// task_aprs.cpp et task_mesh.cpp renseignent ces handles au tout début de
// leur fonction de tâche (xTaskGetCurrentTaskHandle()) ; tant qu'une tâche
// n'est pas démarrée son handle est nullptr et notifyRadioActivityFromISR()
// l'ignore silencieusement.
// ============================================================================

extern TaskHandle_t g_aprs_task_handle;
extern TaskHandle_t g_mesh_task_handle;

void notifyRadioActivityFromISR();

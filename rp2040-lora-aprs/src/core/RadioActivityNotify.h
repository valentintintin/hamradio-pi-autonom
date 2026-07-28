#pragma once

#include <FreeRTOS.h>
#include <task.h>

// ============================================================================
// Handles des tâches APRS/mesh, à réveiller sur activité radio réelle (RX ou
// TX terminé) au lieu du polling en vTaskDelay(1) :
//   - matériel réel : lus directement par aprs/NotifyingRadioLibWrapper.h
//     (sous-classe project-owned de CustomSX1262Wrapper — RadioLibWrapper
//     lui-même, vendoré, n'est pas modifié) depuis le contexte IRQ DIO1.
//   - natif : lus par native_sim/sim/SimCli.cpp et native_sim/web/SimWebBridge.cpp
//     après une injection RX ("sim rx mesh|aprs ...") — pas d'IRQ réelle, mais
//     le même réveil immédiat plutôt que d'attendre le prochain réveil
//     périodique.
//
// tasks/task_aprs.cpp et tasks/task_mesh.cpp renseignent ces handles au tout
// début de leur fonction de tâche (xTaskGetCurrentTaskHandle()) ; tant qu'une
// tâche n'est pas démarrée son handle est nullptr et un xTaskNotifyGive/
// vTaskNotifyGiveFromISR dessus est un no-op silencieux (cf. freertos_compat.cpp
// et le vrai FreeRTOS, tous deux tolérants à un handle nul ici).
//
// aprs/AprsDispatcher.cpp réveille aussi g_aprs_task_handle depuis send()
// (nouveau paquet enfilé) — pas seulement l'IRQ radio.
// ============================================================================

extern TaskHandle_t g_aprs_task_handle;
extern TaskHandle_t g_mesh_task_handle;

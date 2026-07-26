// ============================================================================
// Task CLI — commandes série
//
// Dispatch : commandes "mesh ..." vers MeshCore (MeshcoreRepeater retombe
//            elle-même sur notre CommandHandler si elle ne reconnaît pas la
//            commande — cf MeshcoreRepeater::handleCommand),
//            le reste directement vers notre CommandHandler (get/set/list/...)
// ============================================================================

#include "tasks.h"
#include "core/Log.h"
#include "mesh/MeshcoreRepeater.h"
#include "cli/CommandHandler.h"
#include "aprs/SstvTransmitter.h"
#include "task_heartbeat.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

extern MyMesh the_mesh;
extern CommandHandler command_handler;
extern SstvTransmitter sstv_transmitter;

#define TAG "CLI"
#define CLI_LINE_MAX 160
#define IMAGE_UPLOAD_INACTIVITY_TIMEOUT_MS 10000

// ============================================================================
// Upload binaire d'une image SSTV ("image <n>", série uniquement) — lecture
// par blocs sans le délai de 20ms de la boucle CLI normale (bien trop lent
// pour ~230 Ko), en gardant les heartbeats CLI à jour (watchdog) et un
// timeout d'inactivité pour ne pas rester bloqué si le PC s'arrête en cours.
// ============================================================================
static void receiveImageBinary(uint32_t expected_bytes) {
  static uint8_t buf[256];
  uint32_t received = 0;
  unsigned long last_byte_time = millis();

  while (received < expected_bytes) {
    heartbeat(HB_CLI);

    size_t avail = Serial.available();
    if (avail > 0) {
      size_t to_read = avail > sizeof(buf) ? sizeof(buf) : avail;
      size_t n = Serial.readBytes(buf, to_read);
      if (n > 0) {
        if (!sstv_transmitter.writeImageChunk(buf, n)) {
          LOG_W(F("Erreur écriture, upload annulé"));
          sstv_transmitter.cancelUpload();
          return;
        }
        received += n;
        last_byte_time = millis();
      }
    } else {
      if (millis() - last_byte_time > IMAGE_UPLOAD_INACTIVITY_TIMEOUT_MS) {
        LOG_W(F("Timeout upload (inactivité), annulé"));
        sstv_transmitter.cancelUpload();
        return;
      }
      vTaskDelay(pdMS_TO_TICKS(2));
    }
  }

  LOG_I(F("Upload terminé"));
}

void taskCli(void* params) {
  (void)params;
  LOG_I(TAG, "Prêt. Tapez 'help'.");

  static char line[CLI_LINE_MAX];
  static char reply_buf[CLI_LINE_MAX];
  size_t line_len = 0;

  for (;;) {
    heartbeat(HB_CLI);

    while (Serial.available()) {
      char c = (char)Serial.read();
      if (c == '\r') {
        continue; // ignoré, on ne coupe la ligne que sur '\n'
      }
      if (c == '\n') {
        line[line_len] = '\0';

        // Trim des espaces de tête
        char* cmd = line;
        while (*cmd == ' ') {
          cmd++;
        }

        if (*cmd != '\0') {
          if (strncmp(cmd, "mesh ", 5) == 0) {
            reply_buf[0] = '\0';
            the_mesh.handleCommand(0, cmd + 5, reply_buf);
            if (reply_buf[0]) {
              Serial.printf("  -> %s\n", reply_buf);
            }
            LOG_D(TAG, "MeshCore: %s", cmd + 5);
          } else if (strncmp(cmd, "image ", 6) == 0 && isdigit((unsigned char)cmd[6])) {
            // "image <n>" : upload binaire, série uniquement — intercepté ici
            // (avant CommandHandler) pour basculer la lecture en mode binaire
            // (cf. aprs/SstvTransmitter.h). "image send"/"image cancel" (pas
            // numériques) continuent vers CommandHandler normalement.
            uint32_t announced = (uint32_t)strtoul(cmd + 6, nullptr, 10);
            if (sstv_transmitter.beginImageUpload(announced, &Serial)) {
              LOG_I("CLI IMAGE", "OK, RGB888...");
              receiveImageBinary(announced);
            }
          } else if (!command_handler.execute(cmd, Serial)) {
            LOG_W("CLI", "Commande inconnue: %s (tapez 'help')\n", cmd);
          }
        }

        line_len = 0;
      } else if (line_len < sizeof(line) - 1) {
        line[line_len++] = c;
      }
      // au-delà de CLI_LINE_MAX, les caractères en trop sont silencieusement
      // ignorés jusqu'au prochain '\n' (évite un buffer overflow)
    }
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

#include "tasks.h"
#include "core/Log.h"
#include "mesh/MeshcoreRepeater.h"
#include "cli/CommandHandler.h"
#include "aprs/SstvTransmitter.h"
#include "task_heartbeat.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#ifdef NATIVE_BUILD
#include "SimCli.h"
#endif

extern MyMesh the_mesh;
extern CommandHandler command_handler;
extern SstvTransmitter sstv_transmitter;

#define TAG "CLI"
#define CLI_LINE_MAX 160
#define IMAGE_UPLOAD_INACTIVITY_TIMEOUT_MS 10000

// Lecture par blocs sans le délai de 20ms de la boucle CLI normale (trop lent
// pour ~230 Ko) ; heartbeat() maintenu pour le watchdog.
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
          LOG_W("CLI IMAGE", "Erreur écriture, upload annulé");
          sstv_transmitter.cancelUpload();
          return;
        }
        received += n;
        last_byte_time = millis();
      }
    } else {
      if (millis() - last_byte_time > IMAGE_UPLOAD_INACTIVITY_TIMEOUT_MS) {
        LOG_W("CLI IMAGE", "Timeout upload (inactivité), annulé");
        sstv_transmitter.cancelUpload();
        return;
      }
      vTaskDelay(pdMS_TO_TICKS(2));
    }
  }

  LOG_I("CLI IMAGE", "Upload terminé, tapez 'image send'");
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
        continue;
      }
      if (c == '\n') {
        line[line_len] = '\0';

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
            // Intercepté avant CommandHandler pour basculer en lecture binaire ;
            // "image send"/"cancel" (non numériques) continuent normalement.
            uint32_t announced = (uint32_t)strtoul(cmd + 6, nullptr, 10);
            if (sstv_transmitter.beginImageUpload(announced, &Serial)) {
              LOG_I("CLI IMAGE", "OK, RGB888...");
              receiveImageBinary(announced);
            }
#ifdef NATIVE_BUILD
          } else if (simHandleCommand(cmd, Serial)) {
#endif
          } else if (!command_handler.execute(cmd, Serial)) {
            LOG_W("CLI", "Commande inconnue: %s (tapez 'help')\n", cmd);
          }
        }

        line_len = 0;
      } else if (line_len < sizeof(line) - 1) {
        line[line_len++] = c;
      }
      // au-delà de CLI_LINE_MAX, excédent ignoré jusqu'au prochain '\n'
    }
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

// ============================================================================
// Task CLI — commandes série
//
// Dispatch : commandes "mesh ..." vers MeshCore (MeshcoreRepeater retombe
//            elle-même sur notre CommandHandler si elle ne reconnaît pas la
//            commande — cf MeshcoreRepeater::handleCommand),
//            le reste directement vers notre CommandHandler (get/set/list/...)
// ============================================================================

#include "tasks.h"
#include "config/Log.h"
#include "mesh/MeshcoreRepeater.h"
#include "config/CommandHandler.h"
#include <string.h>

extern MyMesh the_mesh;
extern CommandHandler command_handler;

#define TAG "CLI"
#define CLI_LINE_MAX 160

void taskCli(void* params) {
  (void)params;
  LOG_I(TAG, "Prêt. Tapez 'help'.");

  static char line[CLI_LINE_MAX];
  static char reply_buf[CLI_LINE_MAX];
  size_t line_len = 0;

  for (;;) {
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
          } else if (!command_handler.execute(cmd, Serial)) {
            Serial.printf("Commande inconnue: %s (tapez 'help')\n", cmd);
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

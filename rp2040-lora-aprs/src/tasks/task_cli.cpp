// ============================================================================
// Task CLI — commandes série
//
// Dispatch : commandes "mesh ..." vers MeshCore CommonCLI,
//            le reste vers notre CommandHandler (get/set/list/save/...)
// ============================================================================

#include "tasks.h"
#include "config/Log.h"
#include "mesh/MeshcoreRepeater.h"
#include "config/CommandHandler.h"
#include <string.h>

extern MyMesh the_mesh;
extern CommandHandler command_handler;

#define TAG "CLI"

void taskCli(void* params) {
  (void)params;
  LOG_I(TAG, "Prêt. Tapez 'help'.");

  static char cmd_buf[160];
  static char reply_buf[160];

  for (;;) {
    if (Serial.available()) {
      String cmd = Serial.readStringUntil('\n');
      cmd.trim();

      if (cmd.length() > 0) {
        if (cmd.startsWith("mesh ")) {
          // Dispatch vers MeshCore CommonCLI ; si elle ne reconnaît pas la
          // commande, on retente sur notre CommandHandler (commandes custom :
          // relay, version, aprs, clockdate... accessibles aussi via "mesh ")
          strncpy(cmd_buf, cmd.c_str() + 5, sizeof(cmd_buf) - 1);
          cmd_buf[sizeof(cmd_buf) - 1] = '\0';
          reply_buf[0] = '\0';

          the_mesh.handleCommand(0, cmd_buf, reply_buf);

          if (strcmp(reply_buf, "Unknown command") == 0) {
            if (!command_handler.execute(cmd_buf, Serial)) {
              Serial.printf("Commande inconnue: %s (tapez 'help')\n", cmd_buf);
            }
          } else if (reply_buf[0]) {
            Serial.printf("  -> %s\n", reply_buf);
          }
          LOG_D(TAG, "MeshCore: %s", cmd_buf);
        }
        else if (!command_handler.execute(cmd.c_str(), Serial)) {
          Serial.printf("Commande inconnue: %s (tapez 'help')\n", cmd.c_str());
        }
      }
    }
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

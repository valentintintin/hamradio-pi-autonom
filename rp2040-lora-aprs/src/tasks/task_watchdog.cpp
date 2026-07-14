#include "tasks.h"
#include "config/Log.h"
#include "config/Settings.h"

// ============================================================================
// Task Watchdog — arme et nourrit le watchdog matériel du RP2040
//
// Portée volontairement limitée : une seule tâche nourrit le watchdog, sans
// vérifier la vivacité des autres tâches individuellement. Ça protège contre
// un blocage total du système (deadlock, boucle infinie ailleurs empêchant
// l'ordonnanceur de tourner), mais pas contre une tâche isolée qui se bloque
// pendant que les autres continuent de tourner normalement. Une surveillance
// par tâche (heartbeat par tâche) serait plus complète mais demanderait de
// modifier chaque task_*.cpp — hors scope pour l'instant.
//
// Limite matérielle RP2040 : le watchdog ne peut pas être désarmé une fois
// démarré (rp2040.wdt_begin), donc settings.system.watchdog_enabled n'est lu
// qu'au démarrage ; le changer à chaud ne prend effet qu'après reboot.
// ============================================================================

extern Settings settings;

#define TAG "WDT"
#define WATCHDOG_TIMEOUT_MS      8000  // proche du max matériel RP2040 (~8.3s)
#define WATCHDOG_FEED_PERIOD_MS  2000

void taskWatchdog(void* params) {
  (void)params;

  if (!settings.system.watchdog_enabled) {
    LOG_I(TAG, "Watchdog désactivé (settings.system.watchdog)");
    vTaskDelete(nullptr);
    return;
  }

  rp2040.wdt_begin(WATCHDOG_TIMEOUT_MS);
  LOG_I(TAG, "Watchdog matériel armé (%d ms)", WATCHDOG_TIMEOUT_MS);

  for (;;) {
    rp2040.wdt_reset();
    vTaskDelay(pdMS_TO_TICKS(WATCHDOG_FEED_PERIOD_MS));
  }
}

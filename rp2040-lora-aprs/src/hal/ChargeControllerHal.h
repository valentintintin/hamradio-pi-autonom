#pragma once

#include "Telemetry.h"

// ============================================================================
// ChargeControllerHal — interface commune aux sources de charge solaire qui
// alimentent telemetry.battery_mppt/solar_mppt/mppt_status : la carte MPPT
// I2C (MpptChargerHal) et le Victron VE.Direct (VictronHal) sont deux
// alternatives mutuellement exclusives selon la révision de carte (l'une ou
// l'autre est présente, jamais les deux), mais partagent le même contrat de
// lecture — le code qui se contente de lire la télémétrie (task_sensors,
// task_energy) n'a pas besoin de savoir laquelle est présente.
//
// Les fonctions propres au chip MPPT (watchdog, alerte, seuils de coupure
// matériels) restent des méthodes additionnelles sur MpptChargerHal : Victron
// (simple lien de supervision VE.Direct) ne les supporte pas.
// ============================================================================

class ChargeControllerHal {
public:
  virtual ~ChargeControllerHal() = default;

  virtual bool begin() = 0;
  virtual bool query(TelemetryData& telemetry) = 0;
  virtual bool isInitialized() const = 0;
};

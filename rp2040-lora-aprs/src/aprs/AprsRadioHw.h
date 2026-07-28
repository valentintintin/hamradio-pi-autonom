#pragma once

#include <stdint.h>
#include <stddef.h>

// ============================================================================
// IAprsRadioHw — opérations bas niveau sur le SX1262 433 MHz (aprs_radio_hw)
// nécessaires au basculement LoRa/FSK et à la réception/relais WH65B.
//
// Deux implémentations, une par environnement (même principe que mesh_radio_driver/
// aprs_radio_driver déjà pour le paquet LoRa-APRS lui-même) : `aprs_radio_hw`
// est déclaré ici comme référence abstraite et défini concrètement dans
// variant/target.cpp (AprsRadioHwReal, vrai SX1262 via RadioLib) et
// variant_native/target_native.cpp (AprsRadioHwSim, backée par SimWorld) —
// aucun #ifdef NATIVE_BUILD nécessaire côté appelants (aprs/LoRa433RadioMode.cpp,
// tasks/task_weather.cpp).
// ============================================================================

class IAprsRadioHw {
public:
  virtual ~IAprsRadioHw() = default;

  // Configure le modem en réception FSK (WH65B, 433.92 MHz/8.21 kbps).
  virtual bool switchToFsk() = 0;

  // Restaure la configuration LoRa-APRS normale (depuis settings.radio.aprs_*).
  virtual bool switchToLora() = 0;

  // Écoute une trame WH65B (WH65B_PAYLOAD_LEN octets, cf. LoRa433RadioMode.h)
  // jusqu'à `timeoutMs`. Remplit `outBuf` et `outRssi` si une trame est reçue.
  virtual bool receiveWh65bFrame(uint32_t timeoutMs, uint8_t* outBuf, float* outRssi) = 0;

  // Retransmet une trame WH65B brute (relais protocole, pas une conversion
  // APRS — d'autres stations WH65B à portée écoutent directement ce format).
  virtual bool relayWh65bFrame(const uint8_t* data, size_t len, int8_t powerDbm) = 0;

  // Coupe l'émission (fin de séquence CW/SSTV, cf. aprs/SstvTransmitter.cpp).
  virtual void standby() = 0;
};

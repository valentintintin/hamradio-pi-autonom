#pragma once

#include <Aprs.h>
#include <stdint.h>

// ============================================================================
// AprsEventCallback — événements émis par AprsEngine vers l'application
// (télémétrie, météo, statut, messages entrants).
// ============================================================================
class AprsEventCallback {
public:
  virtual ~AprsEventCallback() = default;

  // Paquet APRS reçu et décodé (tous types confondus)
  virtual void onAprsFrameReceived(const aprs::PacketLite& pkt, float rssi, float snr) {}

  // Message APRS adressé à nous (hors ACK/REJ)
  virtual void onAprsMessageReceived(const char* from, const char* message) {}

  // Fournir les données télémétriques courantes
  virtual void fillTelemetryData(aprs::Telemetry& telemetry) {}

  // Fournir les données météo courantes
  virtual void fillWeatherData(aprs::Weather& weather) {}

  // Fournir le texte de statut courant (reflète l'état de la station)
  virtual void fillStatusText(char* buf, size_t len) { if (len) buf[0] = '\0'; }
};

#pragma once

// ============================================================================
// AprsEngine — Encodage/décodage APRS + digipeat
//
// Porté depuis Communication.cpp (rp2040-lora-aprs-old).
// Découplé du System monolithique : utilise AprsDispatcher pour l'envoi
// et des callbacks pour les événements.
// ============================================================================

#include "AprsDispatcher.h"
#include <Aprs.h>
#include <stdint.h>

// Header LoRa-APRS standard (3 bytes)
#define LORA_APRS_HEADER_0  '<'
#define LORA_APRS_HEADER_1  0xFF
#define LORA_APRS_HEADER_2  0x01
#define LORA_APRS_HEADER_SIZE 3

// Buffer pour frames APRS texte
#define APRS_TEXT_BUFFER_SIZE 256

// ============================================================================
// Config APRS (remplace l'ancien SettingsAprs)
// ============================================================================
struct AprsConfig {
  char callsign[10];
  char destination[10];       // "APRS" par defaut
  char path[32];              // "WIDE1-1" par defaut
  char pathTelemetry[32];     // path spécifique telemetrie
  char symbol;
  char symbolTable;
  double latitude;
  double longitude;
  uint16_t altitude;          // metres
  bool digipeaterEnabled;
  uint16_t telemetrySequenceNumber;
};

// ============================================================================
// Callback pour events APRS
// ============================================================================
class AprsEventCallback {
public:
  // Paquet APRS reçu et décodé
  virtual void onAprsFrameReceived(const AprsPacketLite& pkt, float rssi, float snr) {}

  // Message APRS adressé à nous
  virtual void onAprsMessageReceived(const char* from, const char* message) {}

  // Fournir les données télémétriques
  virtual void fillTelemetryData(AprsPacket& pkt) {}

  // Fournir les données météo
  virtual void fillWeatherData(AprsPacket& pkt) {}
};

// ============================================================================
// AprsEngine
// ============================================================================
class AprsEngine : public AprsRxCallback {
public:
  AprsEngine(AprsDispatcher& dispatcher, AprsConfig& config);

  void setEventCallback(AprsEventCallback* cb) { _event_cb = cb; }

  // --- Envoi ---------------------------------------------------------------
  bool sendPosition(const char* comment);
  bool sendStatus(const char* comment);
  bool sendTelemetry();
  bool sendTelemetryParams();
  bool sendMessage(const char* destination, const char* message, const char* ackToConfirm = nullptr);
  bool sendItem(const char* name, char symbol, char symbolTable, const char* comment,
                double latitude, double longitude, uint16_t altitude, bool alive = true);

  // --- Réception (callback AprsDispatcher) ---------------------------------
  void onAprsPacketReceived(const uint8_t* data, uint8_t len, float rssi, float snr) override;

private:
  AprsDispatcher* _dispatcher;
  AprsConfig* _config;
  AprsEventCallback* _event_cb;

  // Buffers de travail
  AprsPacket _tx_pkt;
  AprsPacketLite _rx_pkt;
  char _text_buf[APRS_TEXT_BUFFER_SIZE];
  uint8_t _raw_buf[APRS_MAX_PACKET_SIZE];

  // Encode et enqueue le paquet via le dispatcher
  bool encodeAndSend(uint8_t priority, uint32_t delay_ms = 0);

  // Prépare les champs communs du paquet TX
  void prepareTxCommon();
  void prepareTxCommonTelemetry();
};

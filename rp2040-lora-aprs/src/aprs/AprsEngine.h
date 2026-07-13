#pragma once

// ============================================================================
// AprsEngine — Encodage/décodage APRS + digipeat, sur SimpleLibAprs
//
// Découplé du System monolithique : utilise AprsDispatcher pour l'envoi
// et des callbacks (AprsEventCallback) pour les événements (télémetrie,
// météo, messages entrants).
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

// Suppression des doublons digipeat — APRS Digipeater Algorithm §4.2a (~30s)
#define APRS_DEDUP_SLOTS       16
#define APRS_DEDUP_WINDOW_MS   30000

// Anti-flood pour les réponses aux requêtes générales ("?APRS?" non dirigées)
#define APRS_GENERAL_QUERY_MIN_INTERVAL_MS 300000  // 5 min

// ============================================================================
// Config APRS (remplace l'ancien SettingsAprs)
// ============================================================================
struct AprsConfig {
  char callsign[10];
  char destination[10];       // "APRS" par defaut
  char path[32];               // "WIDE1-1" par defaut
  char pathTelemetry[32];      // path spécifique telemetrie
  char comment[40];            // commentaire par défaut (position, réponse aux query)
  char symbol;
  char symbolTable;
  double latitude;
  double longitude;
  uint16_t altitude;           // metres
  bool digipeaterEnabled;
  uint16_t telemetrySequenceNumber;
};

// ============================================================================
// Callback pour events APRS
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

// ============================================================================
// AprsEngine
// ============================================================================
class AprsEngine : public AprsRxCallback {
public:
  AprsEngine(AprsDispatcher& dispatcher, AprsConfig& config);

  void setEventCallback(AprsEventCallback* cb) { _event_cb = cb; }

  // --- Envoi ---------------------------------------------------------------
  bool sendPosition(const char* comment);
  bool sendWeather();
  bool sendStatus(const char* text);
  // Demande le texte de statut courant à l'event callback (fillStatusText) et
  // ne transmet que s'il a changé depuis le dernier envoi, ou si
  // `max_interval_ms` s'est écoulé (filet de sécurité "toujours entendu").
  bool sendStatusIfChanged(uint32_t max_interval_ms);
  bool sendTelemetry();
  bool sendTelemetryParams();
  // Message sortant ; ackToAsk (optionnel) demande un accusé de réception au destinataire
  bool sendMessage(const char* destination, const char* message, const char* ackToAsk = nullptr);
  // Accusé de réception standalone pour un message reçu (ackId = "{nn" reçu)
  bool sendAck(const char* destination, const char* ackId);
  bool sendItem(const char* name, char symbol, char symbolTable, const char* comment,
                double latitude, double longitude, uint16_t altitude, bool alive = true);
  // Envoi manuel d'un contenu APRS brut (debug/test), sous notre callsign/path configuré
  bool sendRaw(const char* content);

  // --- Réception (callback AprsDispatcher) ---------------------------------
  void onAprsPacketReceived(const uint8_t* data, uint8_t len, float rssi, float snr) override;

private:
  AprsDispatcher* _dispatcher;
  AprsConfig* _config;
  AprsEventCallback* _event_cb;

  // Buffers de travail
  char _text_buf[APRS_TEXT_BUFFER_SIZE];
  uint8_t _raw_buf[APRS_MAX_PACKET_SIZE];
  aprs::PacketLite _rx_pkt;

  // Suppression des doublons digipeat (§4.2a)
  struct DedupEntry {
    uint32_t hash;
    uint32_t timeMs;
  };
  DedupEntry _dedup[APRS_DEDUP_SLOTS];
  uint32_t frameHash(const aprs::PacketLite& p) const;
  bool isDuplicate(uint32_t hash, uint32_t now) const;
  void remember(uint32_t hash, uint32_t now);

  // Anti-flood requêtes générales
  unsigned long _last_general_query_reply_ms = 0;

  // Suivi du dernier statut envoyé (pour sendStatusIfChanged)
  char _last_status_text[64] = {};
  unsigned long _last_status_sent_ms = 0;

  // Encode un frame déjà écrit dans _text_buf (taille `size`) et l'enqueue
  bool encodeAndSend(size_t size, uint8_t priority, uint32_t delay_ms = 0);

  void handleQuery(const aprs::PacketLite& pkt);
  void handleDigipeat(aprs::PacketLite& pkt);
};

#pragma once

#include "AprsDispatcher.h"
#include "AprsEventCallback.h"
#include "AprsDedup.h"
#include "config/Settings.h"
#include <Aprs.h>
#include <stdint.h>
#include <FreeRTOS.h>
#include <semphr.h>

#define LORA_APRS_HEADER_0  '<'
#define LORA_APRS_HEADER_1  0xFF
#define LORA_APRS_HEADER_2  0x01
#define LORA_APRS_HEADER_SIZE 3

#define APRS_TEXT_BUFFER_SIZE 256

#define APRS_GENERAL_QUERY_MIN_INTERVAL_MS 300000  // 5 min

class AprsEngine : public AprsRxCallback {
public:
  // `settings` doit pointer vers settings.aprs (config live) : pas de copie
  // locale, un "set aprs.xxx" au CLI prend effet immédiatement.
  AprsEngine(AprsDispatcher& dispatcher, AprsSettings& settings);

  void setEventCallback(AprsEventCallback* cb) { _event_cb = cb; }

  // Verrou récursif partagé avec CommandHandler : un message APRS reçu peut
  // déclencher CommandHandler::execute(), qui peut appeler AprsEngine::sendXxx() ;
  // deux verrous distincts créeraient un risque de deadlock AB-BA.
  SemaphoreHandle_t getMutex() const { return _mutex; }

  bool sendPosition(const char* comment);
  bool sendWeather();
  bool sendStatus(const char* text);
  bool sendStatusIfChanged(uint32_t max_interval_ms);
  bool sendTelemetry();
  bool sendTelemetryParams();
  bool sendMessage(const char* destination, const char* message, const char* ackToAsk = nullptr);
  bool sendAck(const char* destination, const char* ackId);
  bool sendItem(const char* name, char symbol, char symbolTable, const char* comment,
                double latitude, double longitude, uint16_t altitude, bool alive = true);
  bool sendRaw(const char* content);

  void onAprsPacketReceived(const uint8_t* data, uint8_t len, float rssi, float snr) override;

private:
  AprsDispatcher* _dispatcher;
  AprsSettings* _settings;
  AprsEventCallback* _event_cb;

  SemaphoreHandle_t _mutex;

  char _text_buf[APRS_TEXT_BUFFER_SIZE];
  uint8_t _raw_buf[APRS_MAX_PACKET_SIZE];
  aprs::PacketLite _rx_pkt;

  uint16_t _telemetry_seq = 0;

  AprsDedup _dedup;

  unsigned long _last_general_query_reply_ms = 0;

  char _last_status_text[64] = {};
  unsigned long _last_status_sent_ms = 0;

  bool encodeAndSend(size_t size, uint8_t priority, uint32_t delay_ms = 0);

  void handleQuery(const aprs::PacketLite& pkt);
  void handleDigipeat(aprs::PacketLite& pkt);
};

#include "AprsEngine.h"
#include "config/Log.h"
#include <string.h>
#include <stdio.h>

#define TAG "APRS-ENG"

// ============================================================================
// Porté depuis Communication.cpp (rp2040-lora-aprs-old)
// ============================================================================

AprsEngine::AprsEngine(AprsDispatcher& dispatcher, AprsConfig& config)
  : _dispatcher(&dispatcher), _config(&config), _event_cb(nullptr)
{
  memset(&_tx_pkt, 0, sizeof(_tx_pkt));
  memset(&_rx_pkt, 0, sizeof(_rx_pkt));
}

// ============================================================================
// Helpers
// ============================================================================

void AprsEngine::prepareTxCommon() {
  Aprs::reset(&_tx_pkt);
  strcpy(_tx_pkt.path, _config->path);
  strcpy(_tx_pkt.source, _config->callsign);
  strcpy(_tx_pkt.destination, _config->destination);
}

void AprsEngine::prepareTxCommonTelemetry() {
  Aprs::reset(&_tx_pkt);
  strcpy(_tx_pkt.path, _config->pathTelemetry);
  strcpy(_tx_pkt.source, _config->callsign);
  strcpy(_tx_pkt.destination, _config->destination);
}

bool AprsEngine::encodeAndSend(uint8_t priority, uint32_t delay_ms) {
  size_t size = Aprs::encode(&_tx_pkt, _text_buf);
  if (!size) return false;

  if (size > APRS_MAX_PACKET_SIZE - LORA_APRS_HEADER_SIZE) {
    LOG_E(TAG, "Paquet trop grand: %d", size);
    return false;
  }

  // Header LoRa-APRS
  _raw_buf[0] = LORA_APRS_HEADER_0;
  _raw_buf[1] = LORA_APRS_HEADER_1;
  _raw_buf[2] = LORA_APRS_HEADER_2;
  memcpy(_raw_buf + LORA_APRS_HEADER_SIZE, _text_buf, size);

  LOG_T(TAG, "TX %d bytes prio=%d", size + LORA_APRS_HEADER_SIZE, priority);
  return _dispatcher->send(_raw_buf, size + LORA_APRS_HEADER_SIZE, priority, delay_ms);
}

// ============================================================================
// Position (avec météo optionnelle)
// ============================================================================
bool AprsEngine::sendPosition(const char* comment) {
  prepareTxCommon();

  _tx_pkt.position.symbol = _config->symbol;
  _tx_pkt.position.overlay = _config->symbolTable;
  _tx_pkt.position.latitude = _config->latitude;
  _tx_pkt.position.longitude = _config->longitude;
  _tx_pkt.position.altitudeFeet = _config->altitude * 3.28f;
  _tx_pkt.position.altitudeInComment = false;
  _tx_pkt.type = Position;

  // Météo si callback fourni
  if (_event_cb) {
    _event_cb->fillWeatherData(_tx_pkt);
  }

  strcpy(_tx_pkt.comment, comment);

  return encodeAndSend(APRS_PRIO_BEACON);
}

// ============================================================================
// Status
// ============================================================================
bool AprsEngine::sendStatus(const char* comment) {
  prepareTxCommon();
  strcpy(_tx_pkt.comment, comment);
  _tx_pkt.type = Status;
  return encodeAndSend(APRS_PRIO_BEACON);
}

// ============================================================================
// Telemetry
// ============================================================================
bool AprsEngine::sendTelemetry() {
  prepareTxCommonTelemetry();

  _tx_pkt.telemetries.telemetrySequenceNumber = ++_config->telemetrySequenceNumber;

  if (_event_cb) {
    _event_cb->fillTelemetryData(_tx_pkt);
  }

  _tx_pkt.type = Telemetry;
  return encodeAndSend(APRS_PRIO_LOW);
}

bool AprsEngine::sendTelemetryParams() {
  prepareTxCommonTelemetry();

  // Remplir les labels/unités via callback
  if (_event_cb) {
    _event_cb->fillTelemetryData(_tx_pkt);
  }

  bool result = false;

  _tx_pkt.type = TelemetryLabel;
  result |= encodeAndSend(APRS_PRIO_LOW, 0);

  _tx_pkt.type = TelemetryUnit;
  result |= encodeAndSend(APRS_PRIO_LOW, 500);

  _tx_pkt.type = TelemetryEquation;
  result |= encodeAndSend(APRS_PRIO_LOW, 1000);

  return result;
}

// ============================================================================
// Message (avec ACK)
// ============================================================================
bool AprsEngine::sendMessage(const char* destination, const char* message, const char* ackToConfirm) {
  prepareTxCommon();

  strcpy(_tx_pkt.message.destination, destination);
  strcpy(_tx_pkt.message.message, message);

  if (ackToConfirm && strlen(ackToConfirm) > 0) {
    strcpy(_tx_pkt.message.ackToConfirm, ackToConfirm);
  }

  _tx_pkt.type = Message;

  // Les ACKs sont prioritaires
  uint8_t prio = (ackToConfirm && strlen(ackToConfirm) > 0)
    ? APRS_PRIO_ACK : APRS_PRIO_BEACON;
  return encodeAndSend(prio);
}

// ============================================================================
// Item
// ============================================================================
bool AprsEngine::sendItem(const char* name, char symbol, char symbolTable,
                          const char* comment, double latitude, double longitude,
                          uint16_t altitude, bool alive) {
  prepareTxCommon();

  _tx_pkt.position.latitude = latitude;
  _tx_pkt.position.longitude = longitude;
  _tx_pkt.position.altitudeFeet = altitude * 3.28f;
  _tx_pkt.position.altitudeInComment = false;
  _tx_pkt.position.symbol = symbol;
  _tx_pkt.position.overlay = symbolTable;
  _tx_pkt.item.active = alive;
  strcpy(_tx_pkt.item.name, name);
  strcpy(_tx_pkt.comment, comment);
  _tx_pkt.type = Item;

  return encodeAndSend(APRS_PRIO_BEACON);
}

// ============================================================================
// Réception APRS — callback depuis AprsDispatcher
// ============================================================================
void AprsEngine::onAprsPacketReceived(const uint8_t* data, uint8_t len, float rssi, float snr) {
  // Vérifier le header LoRa-APRS
  if (len < LORA_APRS_HEADER_SIZE + 1) return;
  if (data[0] != LORA_APRS_HEADER_0 || data[1] != LORA_APRS_HEADER_1 || data[2] != LORA_APRS_HEADER_2) {
    return; // pas un paquet LoRa-APRS
  }

  // Décoder la trame APRS (skip le header 3 bytes)
  const char* payload = reinterpret_cast<const char*>(data + LORA_APRS_HEADER_SIZE);
  if (!Aprs::decode(payload, &_rx_pkt)) {
    return; // décodage échoué
  }

  LOG_D(TAG, "RX de %s RSSI:%.0f SNR:%.1f", _rx_pkt.source, rssi, snr);

  // Ignorer nos propres trames
  if (strcasecmp(_rx_pkt.source, _config->callsign) == 0) return;

  // Ignorer les trames qu'on a déjà digipeatées
  char check[32];
  snprintf(check, sizeof(check), "%s*", _config->callsign);
  if (strcasecmp(_rx_pkt.path, check) == 0) return;

  // Notifier le callback
  if (_event_cb) {
    _event_cb->onAprsFrameReceived(_rx_pkt, rssi, snr);
  }

  // Message adressé à nous ?
  if (strstr(_rx_pkt.message.destination, _config->callsign) != nullptr) {
    if (strlen(_rx_pkt.message.message) > 0) {
      // ACK si demandé
      if (strlen(_rx_pkt.message.ackToConfirm) > 0) {
        sendMessage(_rx_pkt.source, "", _rx_pkt.message.ackToConfirm);
      }

      LOG_I(TAG, "MSG de %s: %s", _rx_pkt.source, _rx_pkt.message.message);
      if (_event_cb) {
        _event_cb->onAprsMessageReceived(_rx_pkt.source, _rx_pkt.message.message);
      }
    }
    return;
  }

  // Digipeat si activé
  if (_config->digipeaterEnabled && Aprs::canBeDigipeated(_rx_pkt.path, _config->callsign)) {
    Aprs::reset(&_tx_pkt);
    strcpy(_tx_pkt.source, _rx_pkt.source);
    strcpy(_tx_pkt.path, _rx_pkt.path);
    strcpy(_tx_pkt.destination, _rx_pkt.destination);
    strcpy(_tx_pkt.content, _rx_pkt.content);
    _tx_pkt.type = RawContent;

    // Délai aléatoire pour éviter les collisions de digipeaters
    uint32_t delay = random(100, 500);
    LOG_I(TAG, "Digipeat %s via %s", _rx_pkt.source, _config->callsign);
    encodeAndSend(APRS_PRIO_DIGIPEAT, delay);
  }
}

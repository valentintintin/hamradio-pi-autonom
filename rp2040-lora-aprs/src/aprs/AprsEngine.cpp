#include "AprsEngine.h"
#include "config/Log.h"
#include <string.h>
#include <stdio.h>

#define TAG "APRS-ENG"

// ============================================================================
// Construction
// ============================================================================

AprsEngine::AprsEngine(AprsDispatcher& dispatcher, AprsConfig& config)
  : _dispatcher(&dispatcher), _config(&config), _event_cb(nullptr)
{
  memset(_dedup, 0, sizeof(_dedup));
}

// ============================================================================
// Encode un frame déjà écrit dans _text_buf (taille `size`) et l'enqueue
// via le dispatcher, avec le header LoRa-APRS 3 octets.
// ============================================================================
bool AprsEngine::encodeAndSend(size_t size, uint8_t priority, uint32_t delay_ms) {
  if (size == 0) return false;

  if (size > APRS_MAX_PACKET_SIZE - LORA_APRS_HEADER_SIZE) {
    LOG_E(TAG, "Paquet trop grand: %u", (unsigned)size);
    return false;
  }

  _raw_buf[0] = LORA_APRS_HEADER_0;
  _raw_buf[1] = LORA_APRS_HEADER_1;
  _raw_buf[2] = LORA_APRS_HEADER_2;
  memcpy(_raw_buf + LORA_APRS_HEADER_SIZE, _text_buf, size);

  LOG_T(TAG, "TX %u bytes prio=%d", (unsigned)(size + LORA_APRS_HEADER_SIZE), priority);
  return _dispatcher->send(_raw_buf, size + LORA_APRS_HEADER_SIZE, priority, delay_ms);
}

// ============================================================================
// Position (compressée, sans météo)
// ============================================================================
bool AprsEngine::sendPosition(const char* comment) {
  aprs::Position position;
  position.symbol = _config->symbol;
  position.overlay = _config->symbolTable;
  position.latitude = _config->latitude;
  position.longitude = _config->longitude;
  position.altitudeFeet = _config->altitude * 3.28084;
  position.altitudeInComment = false;

  size_t written = 0;
  aprs::Result r = aprs::encodePosition(_config->callsign, _config->destination, _config->path,
                                        position, nullptr, nullptr, comment,
                                        _text_buf, sizeof(_text_buf), &written);
  if (r != aprs::Result::Ok) return false;

  return encodeAndSend(written, APRS_PRIO_BEACON);
}

// ============================================================================
// Météo — position report avec le payload Weather (symbole '_' forcé par
// l'encodeur dès qu'un Weather est fourni, APRS101 ch.12)
// ============================================================================
bool AprsEngine::sendWeather() {
  aprs::Position position;
  position.latitude = _config->latitude;
  position.longitude = _config->longitude;
  position.overlay = _config->symbolTable;

  aprs::Weather weather;
  if (_event_cb) {
    _event_cb->fillWeatherData(weather);
  }

  size_t written = 0;
  aprs::Result r = aprs::encodePosition(_config->callsign, _config->destination, _config->path,
                                        position, &weather, nullptr, nullptr,
                                        _text_buf, sizeof(_text_buf), &written);
  if (r != aprs::Result::Ok) return false;

  return encodeAndSend(written, APRS_PRIO_BEACON);
}

// ============================================================================
// Status
// ============================================================================
bool AprsEngine::sendStatus(const char* text) {
  size_t written = 0;
  aprs::Result r = aprs::encodeStatus(_config->callsign, _config->destination, _config->path,
                                      text, _text_buf, sizeof(_text_buf), &written);
  if (r != aprs::Result::Ok) return false;

  return encodeAndSend(written, APRS_PRIO_BEACON);
}

bool AprsEngine::sendStatusIfChanged(uint32_t max_interval_ms) {
  if (!_event_cb) return false;

  char text[64];
  text[0] = '\0';
  _event_cb->fillStatusText(text, sizeof(text));
  if (!text[0]) return false;

  unsigned long now = millis();
  bool changed = strcmp(text, _last_status_text) != 0;
  bool fallback_due = _last_status_sent_ms != 0 && (now - _last_status_sent_ms) >= max_interval_ms;

  if (_last_status_sent_ms != 0 && !changed && !fallback_due) return false;

  strncpy(_last_status_text, text, sizeof(_last_status_text) - 1);
  _last_status_text[sizeof(_last_status_text) - 1] = '\0';
  _last_status_sent_ms = now;

  return sendStatus(text);
}

// ============================================================================
// Telemetry
// ============================================================================
bool AprsEngine::sendTelemetry() {
  aprs::Telemetry telemetry;
  telemetry.sequenceNumber = ++_config->telemetrySequenceNumber;

  if (_event_cb) {
    _event_cb->fillTelemetryData(telemetry);
  }

  size_t written = 0;
  aprs::Result r = aprs::encodeTelemetryData(_config->callsign, _config->destination,
                                             _config->pathTelemetry, telemetry, nullptr,
                                             _text_buf, sizeof(_text_buf), &written);
  if (r != aprs::Result::Ok) return false;

  return encodeAndSend(written, APRS_PRIO_LOW);
}

bool AprsEngine::sendTelemetryParams() {
  aprs::Telemetry telemetry;
  if (_event_cb) {
    _event_cb->fillTelemetryData(telemetry);
  }

  bool ok = true;
  size_t written = 0;

  aprs::Result r = aprs::encodeTelemetryLabel(_config->callsign, _config->destination,
                                              _config->pathTelemetry, telemetry,
                                              _text_buf, sizeof(_text_buf), &written);
  ok = (r == aprs::Result::Ok) && encodeAndSend(written, APRS_PRIO_LOW, 0);

  r = aprs::encodeTelemetryUnit(_config->callsign, _config->destination,
                                _config->pathTelemetry, telemetry,
                                _text_buf, sizeof(_text_buf), &written);
  ok = ((r == aprs::Result::Ok) && encodeAndSend(written, APRS_PRIO_LOW, 500)) && ok;

  r = aprs::encodeTelemetryEquation(_config->callsign, _config->destination,
                                    _config->pathTelemetry, telemetry,
                                    _text_buf, sizeof(_text_buf), &written);
  ok = ((r == aprs::Result::Ok) && encodeAndSend(written, APRS_PRIO_LOW, 1000)) && ok;

  r = aprs::encodeTelemetryBitSense(_config->callsign, _config->destination,
                                    _config->pathTelemetry, telemetry,
                                    _text_buf, sizeof(_text_buf), &written);
  ok = ((r == aprs::Result::Ok) && encodeAndSend(written, APRS_PRIO_LOW, 1500)) && ok;

  return ok;
}

// ============================================================================
// Message sortant (avec demande d'accusé de réception optionnelle)
// ============================================================================
bool AprsEngine::sendMessage(const char* destination, const char* message, const char* ackToAsk) {
  aprs::Message msg;
  strncpy(msg.destination, destination, sizeof(msg.destination) - 1);
  strncpy(msg.message, message, sizeof(msg.message) - 1);
  if (ackToAsk && ackToAsk[0]) {
    strncpy(msg.ackToAsk, ackToAsk, sizeof(msg.ackToAsk) - 1);
  }

  size_t written = 0;
  aprs::Result r = aprs::encodeMessage(_config->callsign, _config->destination, _config->path,
                                       msg, _text_buf, sizeof(_text_buf), &written);
  if (r != aprs::Result::Ok) return false;

  return encodeAndSend(written, APRS_PRIO_BEACON);
}

// ============================================================================
// Accusé de réception standalone (répond à une demande "{nn" reçue)
// ============================================================================
bool AprsEngine::sendAck(const char* destination, const char* ackId) {
  aprs::Message msg;
  strncpy(msg.destination, destination, sizeof(msg.destination) - 1);
  strncpy(msg.ackToConfirm, ackId, sizeof(msg.ackToConfirm) - 1);

  size_t written = 0;
  aprs::Result r = aprs::encodeMessage(_config->callsign, _config->destination, _config->path,
                                       msg, _text_buf, sizeof(_text_buf), &written);
  if (r != aprs::Result::Ok) return false;

  return encodeAndSend(written, APRS_PRIO_ACK);
}

// ============================================================================
// Item
// ============================================================================
bool AprsEngine::sendItem(const char* name, char symbol, char symbolTable,
                          const char* comment, double latitude, double longitude,
                          uint16_t altitude, bool alive) {
  aprs::ObjectItem item;
  strncpy(item.name, name, sizeof(item.name) - 1);
  item.active = alive;

  aprs::Position position;
  position.latitude = latitude;
  position.longitude = longitude;
  position.symbol = symbol;
  position.overlay = symbolTable;
  position.altitudeFeet = altitude * 3.28084;
  position.altitudeInComment = true;

  size_t written = 0;
  aprs::Result r = aprs::encodeObjectItem(_config->callsign, _config->destination, _config->path,
                                          aprs::PacketType::Item, item, position, comment,
                                          _text_buf, sizeof(_text_buf), &written);
  if (r != aprs::Result::Ok) return false;

  return encodeAndSend(written, APRS_PRIO_BEACON);
}

// ============================================================================
// Envoi manuel d'un contenu brut (debug/test) sous notre callsign/path
// ============================================================================
bool AprsEngine::sendRaw(const char* content) {
  if (!content || !content[0]) return false;

  size_t written = 0;
  aprs::Result r = aprs::encodeRaw(_config->callsign, _config->destination, _config->path,
                                   content, _text_buf, sizeof(_text_buf), &written);
  if (r != aprs::Result::Ok) return false;

  return encodeAndSend(written, APRS_PRIO_BEACON);
}

// ============================================================================
// Digipeat — APRS Digipeater Algorithm (WB2OSZ, APRS Foundation, 2024-2025)
// ============================================================================
uint32_t AprsEngine::frameHash(const aprs::PacketLite& p) const {
  // Callsign destination sans SSID, pour ne pas distinguer deux copies de la
  // même trame reçues avec des adresses AX.25 destination légèrement différentes.
  char destNoSsid[aprs::kCallsignLength + 1];
  size_t i = 0;
  for (; p.destination[i] && p.destination[i] != '-' && i < sizeof(destNoSsid) - 1; i++) {
    destNoSsid[i] = p.destination[i];
  }
  destNoSsid[i] = '\0';

  uint32_t h = 5381;
  for (const char* s = p.source;   *s; s++) h = (h * 33) ^ (uint8_t)*s;
  for (const char* s = destNoSsid; *s; s++) h = (h * 33) ^ (uint8_t)*s;
  for (const char* s = p.content;  *s; s++) h = (h * 33) ^ (uint8_t)*s;
  return h;
}

bool AprsEngine::isDuplicate(uint32_t hash, uint32_t now) const {
  for (uint8_t i = 0; i < APRS_DEDUP_SLOTS; i++) {
    if (_dedup[i].hash == hash && (now - _dedup[i].timeMs) < APRS_DEDUP_WINDOW_MS) {
      return true;
    }
  }
  return false;
}

void AprsEngine::remember(uint32_t hash, uint32_t now) {
  uint8_t oldest = 0;
  for (uint8_t i = 1; i < APRS_DEDUP_SLOTS; i++) {
    if (_dedup[i].timeMs < _dedup[oldest].timeMs) oldest = i;
  }
  _dedup[oldest].hash = hash;
  _dedup[oldest].timeMs = now;
}

void AprsEngine::handleDigipeat(aprs::PacketLite& pkt) {
  if (!_config->digipeaterEnabled) return;

  // §4.2b : ne jamais relayer nos propres trames
  if (strcasecmp(pkt.source, _config->callsign) == 0) return;

  // §4.1 + §4.3 : sommes-nous le premier hop non utilisé du path ? Si oui,
  // canBeDigipeated réécrit pkt.path en place.
  if (!aprs::canBeDigipeated(pkt.path, sizeof(pkt.path), _config->callsign)) return;

  // §4.2a : suppression des doublons entendus dans les ~30 dernières secondes
  uint32_t now = millis();
  uint32_t hash = frameHash(pkt);
  if (isDuplicate(hash, now)) return;

  // Reconstruit la trame "SOURCE>DEST[,PATH]:CONTENU" avec le path réécrit
  size_t written = 0;
  aprs::Result r = aprs::encodeRaw(pkt.source, pkt.destination, pkt.path[0] ? pkt.path : nullptr,
                                   pkt.content, _text_buf, sizeof(_text_buf), &written);
  if (r != aprs::Result::Ok) return;

  remember(hash, now);

  // Petit délai aléatoire pour limiter les collisions entre digipeaters qui
  // reçoivent tous la même trame en même temps.
  uint32_t delay = random(100, 500);
  LOG_I(TAG, "Digipeat %s via %s", pkt.source, _config->callsign);
  encodeAndSend(written, APRS_PRIO_DIGIPEAT, delay);
}

// ============================================================================
// Query — répond aux requêtes "?type?" (générales) ou ":CALL:?type?" (dirigées)
//
// Décider s'il faut répondre et comment est de la responsabilité de
// l'application (la lib ne fait que décoder) — cf APRS101 ch.15.
// ============================================================================
void AprsEngine::handleQuery(const aprs::PacketLite& pkt) {
  aprs::Query query;
  if (!aprs::decodeQuery(pkt, query)) return;

  bool directed = query.destination[0] != '\0';
  if (directed && strcasecmp(query.destination, _config->callsign) != 0) {
    return; // adressée à une autre station
  }

  if (!directed) {
    // Requête générale (broadcast) : anti-flood pour éviter qu'un "?APRS?"
    // entendu par plusieurs stations ne déclenche une salve de réponses.
    unsigned long now = millis();
    if (_last_general_query_reply_ms != 0 &&
        (now - _last_general_query_reply_ms) < APRS_GENERAL_QUERY_MIN_INTERVAL_MS) {
      LOG_T(TAG, "Query générale %s ignorée (anti-flood)", query.type);
      return;
    }
    _last_general_query_reply_ms = now;
  }

  LOG_I(TAG, "Query %s de %s (%s)", query.type, pkt.source, directed ? "dirigée" : "générale");

  if (strcasecmp(query.type, "APRS") == 0 || strcasecmp(query.type, "APRSP") == 0) {
    sendPosition(_config->comment);
  } else if (strcasecmp(query.type, "WX") == 0) {
    sendWeather();
  } else if (strcasecmp(query.type, "PING") == 0) {
    if (directed) sendMessage(pkt.source, "PONG");
  } else {
    LOG_D(TAG, "Query %s non supportée", query.type);
  }
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

  // Copier le payload dans un buffer local NUL-terminé : `data` ne l'est pas
  // garanti (bytes bruts radio), et aprs::decode attend un C-string.
  char payload[APRS_TEXT_BUFFER_SIZE];
  size_t payload_len = (size_t)len - LORA_APRS_HEADER_SIZE;
  if (payload_len >= sizeof(payload)) payload_len = sizeof(payload) - 1;
  memcpy(payload, data + LORA_APRS_HEADER_SIZE, payload_len);
  payload[payload_len] = '\0';

  if (!aprs::decode(payload, _rx_pkt)) {
    return; // décodage échoué
  }

  LOG_D(TAG, "RX de %s RSSI:%.0f SNR:%.1f", _rx_pkt.source, rssi, snr);

  // Ignorer nos propres trames
  if (strcasecmp(_rx_pkt.source, _config->callsign) == 0) return;

  // Notifier le callback générique
  if (_event_cb) {
    _event_cb->onAprsFrameReceived(_rx_pkt, rssi, snr);
  }

  if (_rx_pkt.type == aprs::PacketType::Message) {
    aprs::Message message;
    if (aprs::decodeMessage(_rx_pkt, message) &&
        strcasecmp(message.destination, _config->callsign) == 0) {

      // Le correspondant demande un accusé de réception ("{nn")
      if (message.ackToConfirm[0]) {
        sendAck(_rx_pkt.source, message.ackToConfirm);
      }

      // Message réel (pas juste un ACK/REJ pour un de nos envois)
      if (message.message[0] && !message.ackConfirmed[0] && !message.ackRejected[0]) {
        LOG_I(TAG, "MSG de %s: %s", _rx_pkt.source, message.message);
        if (_event_cb) {
          _event_cb->onAprsMessageReceived(_rx_pkt.source, message.message);
        }
      }
      return; // message pour nous : pas de digipeat plus loin
    }
  } else if (_rx_pkt.type == aprs::PacketType::Query) {
    handleQuery(_rx_pkt);
  }

  // Digipeat (path-based, indépendant du type de contenu)
  handleDigipeat(_rx_pkt);
}

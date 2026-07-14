#include "AprsEngine.h"
#include "config/Log.h"
#include <string.h>
#include <stdio.h>

#define TAG "APRS-ENG"

// ============================================================================
// Verrou récursif — sendXxx()/onAprsPacketReceived() sont appelés depuis
// plusieurs tâches FreeRTOS (beacon, CLI, mesh, dispatcher APRS) et partagent
// _text_buf/_raw_buf/_rx_pkt. Récursif car onAprsPacketReceived (déjà
// verrouillé) peut lui-même appeler sendPosition/sendWeather/sendMessage via
// handleQuery()/le traitement des messages.
// ============================================================================
namespace {
struct EngineLockGuard {
  SemaphoreHandle_t sem;
  explicit EngineLockGuard(SemaphoreHandle_t s) : sem(s) {
    xSemaphoreTakeRecursive(sem, portMAX_DELAY);
  }
  ~EngineLockGuard() {
    xSemaphoreGiveRecursive(sem);
  }
};
}  // namespace

// ============================================================================
// Construction
// ============================================================================

AprsEngine::AprsEngine(AprsDispatcher& dispatcher, AprsSettings& settings)
  : _dispatcher(&dispatcher), _settings(&settings), _event_cb(nullptr)
{
  memset(_dedup, 0, sizeof(_dedup));
  _mutex = xSemaphoreCreateRecursiveMutex();
}

// ============================================================================
// Encode un frame déjà écrit dans _text_buf (taille `size`) et l'enqueue
// via le dispatcher, avec le header LoRa-APRS 3 octets.
// ============================================================================
bool AprsEngine::encodeAndSend(size_t size, uint8_t priority, uint32_t delay_ms) {
  if (size == 0) {
    return false;
  }

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
  EngineLockGuard lock(_mutex);

  aprs::Position position;
  position.symbol = _settings->symbol;
  position.overlay = _settings->symbolTable;
  position.latitude = _settings->latitude;
  position.longitude = _settings->longitude;
  position.altitudeFeet = _settings->altitude * 3.28084;
  position.altitudeInComment = false;

  size_t written = 0;
  aprs::Result r = aprs::encodePosition(_settings->callsign, _settings->destination, _settings->path,
                                        position, nullptr, nullptr, comment,
                                        _text_buf, sizeof(_text_buf), &written);
  if (r != aprs::Result::Ok) {
    return false;
  }

  return encodeAndSend(written, APRS_PRIO_BEACON);
}

// ============================================================================
// Météo — position report avec le payload Weather (symbole '_' forcé par
// l'encodeur dès qu'un Weather est fourni, APRS101 ch.12)
// ============================================================================
bool AprsEngine::sendWeather() {
  EngineLockGuard lock(_mutex);

  aprs::Position position;
  position.latitude = _settings->latitude;
  position.longitude = _settings->longitude;
  position.overlay = _settings->symbolTable;

  aprs::Weather weather;
  if (_event_cb) {
    _event_cb->fillWeatherData(weather);
  }

  size_t written = 0;
  aprs::Result r = aprs::encodePosition(_settings->callsign, _settings->destination, _settings->path,
                                        position, &weather, nullptr, nullptr,
                                        _text_buf, sizeof(_text_buf), &written);
  if (r != aprs::Result::Ok) {
    return false;
  }

  return encodeAndSend(written, APRS_PRIO_BEACON);
}

// ============================================================================
// Status
// ============================================================================
bool AprsEngine::sendStatus(const char* text) {
  EngineLockGuard lock(_mutex);

  size_t written = 0;
  aprs::Result r = aprs::encodeStatus(_settings->callsign, _settings->destination, _settings->path,
                                      text, _text_buf, sizeof(_text_buf), &written);
  if (r != aprs::Result::Ok) {
    return false;
  }

  return encodeAndSend(written, APRS_PRIO_BEACON);
}

bool AprsEngine::sendStatusIfChanged(uint32_t max_interval_ms) {
  EngineLockGuard lock(_mutex);

  if (!_event_cb) {
    return false;
  }

  char text[64];
  text[0] = '\0';
  _event_cb->fillStatusText(text, sizeof(text));
  if (!text[0]) {
    return false;
  }

  unsigned long now = millis();
  bool changed = strcmp(text, _last_status_text) != 0;
  bool fallback_due = _last_status_sent_ms != 0 && (now - _last_status_sent_ms) >= max_interval_ms;

  if (_last_status_sent_ms != 0 && !changed && !fallback_due) {
    return false;
  }

  strncpy(_last_status_text, text, sizeof(_last_status_text) - 1);
  _last_status_text[sizeof(_last_status_text) - 1] = '\0';
  _last_status_sent_ms = now;

  return sendStatus(text);
}

// ============================================================================
// Telemetry
// ============================================================================
bool AprsEngine::sendTelemetry() {
  EngineLockGuard lock(_mutex);

  aprs::Telemetry telemetry;
  telemetry.sequenceNumber = ++_telemetry_seq;

  if (_event_cb) {
    _event_cb->fillTelemetryData(telemetry);
  }

  size_t written = 0;
  aprs::Result r = aprs::encodeTelemetryData(_settings->callsign, _settings->destination,
                                             _settings->pathTelemetry, telemetry, nullptr,
                                             _text_buf, sizeof(_text_buf), &written);
  if (r != aprs::Result::Ok) {
    return false;
  }

  return encodeAndSend(written, APRS_PRIO_LOW);
}

bool AprsEngine::sendTelemetryParams() {
  EngineLockGuard lock(_mutex);

  aprs::Telemetry telemetry;
  if (_event_cb) {
    _event_cb->fillTelemetryData(telemetry);
  }

  bool ok = true;
  size_t written = 0;

  aprs::Result r = aprs::encodeTelemetryLabel(_settings->callsign, _settings->destination,
                                              _settings->pathTelemetry, telemetry,
                                              _text_buf, sizeof(_text_buf), &written);
  ok = (r == aprs::Result::Ok) && encodeAndSend(written, APRS_PRIO_LOW, 0);

  r = aprs::encodeTelemetryUnit(_settings->callsign, _settings->destination,
                                _settings->pathTelemetry, telemetry,
                                _text_buf, sizeof(_text_buf), &written);
  ok = ((r == aprs::Result::Ok) && encodeAndSend(written, APRS_PRIO_LOW, 500)) && ok;

  r = aprs::encodeTelemetryEquation(_settings->callsign, _settings->destination,
                                    _settings->pathTelemetry, telemetry,
                                    _text_buf, sizeof(_text_buf), &written);
  ok = ((r == aprs::Result::Ok) && encodeAndSend(written, APRS_PRIO_LOW, 1000)) && ok;

  r = aprs::encodeTelemetryBitSense(_settings->callsign, _settings->destination,
                                    _settings->pathTelemetry, telemetry,
                                    _text_buf, sizeof(_text_buf), &written);
  ok = ((r == aprs::Result::Ok) && encodeAndSend(written, APRS_PRIO_LOW, 1500)) && ok;

  return ok;
}

// ============================================================================
// Message sortant (avec demande d'accusé de réception optionnelle)
// ============================================================================
bool AprsEngine::sendMessage(const char* destination, const char* message, const char* ackToAsk) {
  EngineLockGuard lock(_mutex);

  aprs::Message msg;
  strncpy(msg.destination, destination, sizeof(msg.destination) - 1);
  strncpy(msg.message, message, sizeof(msg.message) - 1);
  if (ackToAsk && ackToAsk[0]) {
    strncpy(msg.ackToAsk, ackToAsk, sizeof(msg.ackToAsk) - 1);
  }

  size_t written = 0;
  aprs::Result r = aprs::encodeMessage(_settings->callsign, _settings->destination, _settings->path,
                                       msg, _text_buf, sizeof(_text_buf), &written);
  if (r != aprs::Result::Ok) {
    return false;
  }

  return encodeAndSend(written, APRS_PRIO_BEACON);
}

// ============================================================================
// Accusé de réception standalone (répond à une demande "{nn" reçue)
// ============================================================================
bool AprsEngine::sendAck(const char* destination, const char* ackId) {
  EngineLockGuard lock(_mutex);

  aprs::Message msg;
  strncpy(msg.destination, destination, sizeof(msg.destination) - 1);
  strncpy(msg.ackToConfirm, ackId, sizeof(msg.ackToConfirm) - 1);

  size_t written = 0;
  aprs::Result r = aprs::encodeMessage(_settings->callsign, _settings->destination, _settings->path,
                                       msg, _text_buf, sizeof(_text_buf), &written);
  if (r != aprs::Result::Ok) {
    return false;
  }

  return encodeAndSend(written, APRS_PRIO_ACK);
}

// ============================================================================
// Item
// ============================================================================
bool AprsEngine::sendItem(const char* name, char symbol, char symbolTable,
                          const char* comment, double latitude, double longitude,
                          uint16_t altitude, bool alive) {
  EngineLockGuard lock(_mutex);

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
  aprs::Result r = aprs::encodeObjectItem(_settings->callsign, _settings->destination, _settings->path,
                                          aprs::PacketType::Item, item, position, comment,
                                          _text_buf, sizeof(_text_buf), &written);
  if (r != aprs::Result::Ok) {
    return false;
  }

  return encodeAndSend(written, APRS_PRIO_BEACON);
}

// ============================================================================
// Envoi manuel d'un contenu brut (debug/test) sous notre callsign/path
// ============================================================================
bool AprsEngine::sendRaw(const char* content) {
  EngineLockGuard lock(_mutex);

  if (!content || !content[0]) {
    return false;
  }

  size_t written = 0;
  aprs::Result r = aprs::encodeRaw(_settings->callsign, _settings->destination, _settings->path,
                                   content, _text_buf, sizeof(_text_buf), &written);
  if (r != aprs::Result::Ok) {
    return false;
  }

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
  for (const char* s = p.source; *s; s++) {
    h = (h * 33) ^ (uint8_t)*s;
  }
  for (const char* s = destNoSsid; *s; s++) {
    h = (h * 33) ^ (uint8_t)*s;
  }
  for (const char* s = p.content; *s; s++) {
    h = (h * 33) ^ (uint8_t)*s;
  }
  return h;
}

bool AprsEngine::isDuplicate(uint32_t hash, uint32_t now) const {
  for (uint8_t i = 0; i < APRS_DEDUP_SLOTS; i++) {
    if (_dedup[i].valid && _dedup[i].hash == hash && (now - _dedup[i].timeMs) < APRS_DEDUP_WINDOW_MS) {
      return true;
    }
  }
  return false;
}

void AprsEngine::remember(uint32_t hash, uint32_t now) {
  // Un slot jamais utilisé (valid=false) est toujours le meilleur candidat à
  // remplacer, avant même de regarder les timestamps des slots déjà occupés.
  uint8_t oldest = 0;
  bool oldest_valid = _dedup[0].valid;
  for (uint8_t i = 1; i < APRS_DEDUP_SLOTS; i++) {
    if (!_dedup[i].valid) {
      oldest = i;
      oldest_valid = false;
      break;
    }
    if (oldest_valid && _dedup[i].timeMs < _dedup[oldest].timeMs) {
      oldest = i;
    }
  }
  _dedup[oldest].hash = hash;
  _dedup[oldest].timeMs = now;
  _dedup[oldest].valid = true;
}

void AprsEngine::handleDigipeat(aprs::PacketLite& pkt) {
  if (!_settings->digipeaterEnabled) {
    return;
  }

  // §4.2b : ne jamais relayer nos propres trames
  if (strcasecmp(pkt.source, _settings->callsign) == 0) {
    return;
  }

  // §4.1 + §4.3 : sommes-nous le premier hop non utilisé du path ? Si oui,
  // canBeDigipeated réécrit pkt.path en place.
  if (!aprs::canBeDigipeated(pkt.path, sizeof(pkt.path), _settings->callsign)) {
    return;
  }

  // §4.2a : suppression des doublons entendus dans les ~30 dernières secondes
  uint32_t now = millis();
  uint32_t hash = frameHash(pkt);
  if (isDuplicate(hash, now)) {
    return;
  }

  // Reconstruit la trame "SOURCE>DEST[,PATH]:CONTENU" avec le path réécrit
  size_t written = 0;
  aprs::Result r = aprs::encodeRaw(pkt.source, pkt.destination, pkt.path[0] ? pkt.path : nullptr,
                                   pkt.content, _text_buf, sizeof(_text_buf), &written);
  if (r != aprs::Result::Ok) {
    return;
  }

  remember(hash, now);

  // Petit délai aléatoire pour limiter les collisions entre digipeaters qui
  // reçoivent tous la même trame en même temps.
  uint32_t delay = random(100, 500);
  LOG_I(TAG, "Digipeat %s via %s", pkt.source, _settings->callsign);
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
  if (!aprs::decodeQuery(pkt, query)) {
    return;
  }

  bool directed = query.destination[0] != '\0';
  if (directed && strcasecmp(query.destination, _settings->callsign) != 0) {
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
    sendPosition(_settings->comment);
  } else if (strcasecmp(query.type, "WX") == 0) {
    sendWeather();
  } else if (strcasecmp(query.type, "PING") == 0) {
    if (directed) {
      sendMessage(pkt.source, "PONG");
    }
  } else {
    LOG_D(TAG, "Query %s non supportée", query.type);
  }
}

// ============================================================================
// Réception APRS — callback depuis AprsDispatcher
// ============================================================================
void AprsEngine::onAprsPacketReceived(const uint8_t* data, uint8_t len, float rssi, float snr) {
  EngineLockGuard lock(_mutex);

  // Vérifier le header LoRa-APRS
  if (len < LORA_APRS_HEADER_SIZE + 1) {
    return;
  }
  if (data[0] != LORA_APRS_HEADER_0 || data[1] != LORA_APRS_HEADER_1 || data[2] != LORA_APRS_HEADER_2) {
    return; // pas un paquet LoRa-APRS
  }

  // Copier le payload dans un buffer local NUL-terminé : `data` ne l'est pas
  // garanti (bytes bruts radio), et aprs::decode attend un C-string.
  char payload[APRS_TEXT_BUFFER_SIZE];
  size_t payload_len = (size_t)len - LORA_APRS_HEADER_SIZE;
  if (payload_len >= sizeof(payload)) {
    payload_len = sizeof(payload) - 1;
  }
  memcpy(payload, data + LORA_APRS_HEADER_SIZE, payload_len);
  payload[payload_len] = '\0';

  if (!aprs::decode(payload, _rx_pkt)) {
    return; // décodage échoué
  }

  LOG_D(TAG, "RX de %s RSSI:%.0f SNR:%.1f", _rx_pkt.source, rssi, snr);

  // Ignorer nos propres trames
  if (strcasecmp(_rx_pkt.source, _settings->callsign) == 0) {
    return;
  }

  // Notifier le callback générique
  if (_event_cb) {
    _event_cb->onAprsFrameReceived(_rx_pkt, rssi, snr);
  }

  if (_rx_pkt.type == aprs::PacketType::Message) {
    aprs::Message message;
    if (aprs::decodeMessage(_rx_pkt, message) &&
        strcasecmp(message.destination, _settings->callsign) == 0) {

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

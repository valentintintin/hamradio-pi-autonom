#include "MyAprsDispatcher.h"

#include <SerialUSB.h>

#if MESH_PACKET_LOGGING
  #include <Arduino.h>
#endif

#include <math.h>

namespace mine {

#define MAX_RX_DELAY_MILLIS        32000  // 32 seconds

#ifndef NOISE_FLOOR_CALIB_INTERVAL
  #define NOISE_FLOOR_CALIB_INTERVAL   2000     // 2 seconds
#endif

void MyAprsDispatcher::begin() {
  _err_flags = 0;
  radio_nonrx_start = _ms->getMillis();

  _radio->begin();
  prev_isrecv_mode = _radio->isInRecvMode();

  resetStats();
}

int MyAprsDispatcher::calcRxDelay(float score, uint32_t air_time) const {
  return (int) ((pow(10, 0.85f - score) - 1.0) * air_time);
}

uint32_t MyAprsDispatcher::getCADFailRetryDelay() const {
  return 200;
}
uint32_t MyAprsDispatcher::getCADFailMaxDuration() const {
  return 4000;   // 4 seconds
}

void MyAprsDispatcher::loop() {
  if (millisHasNowPassed(next_floor_calib_time)) {
    _radio->triggerNoiseFloorCalibrate(getInterferenceThreshold());
    next_floor_calib_time = futureMillis(NOISE_FLOOR_CALIB_INTERVAL);
  }
  _radio->loop();

  // check for radio 'stuck' in mode other than Rx
  bool is_recv = _radio->isInRecvMode();
  if (is_recv != prev_isrecv_mode) {
    prev_isrecv_mode = is_recv;
    if (!is_recv) {
      radio_nonrx_start = _ms->getMillis();
    }
  }
  if (!is_recv && _ms->getMillis() - radio_nonrx_start > 8000) {   // radio has not been in Rx mode for 8 seconds!
    _err_flags |= ERR_EVENT_STARTRX_TIMEOUT;
  }

  if (outbound) {  // waiting for outbound send to be completed
    if (_radio->isSendComplete()) {
      const long t = _ms->getMillis() - outbound_start;
      total_air_time += t;
      //Serial.print("  airtime="); Serial.println(t);

      _radio->onSendFinished();
      logTx(outbound);
      n_sent++;
      releasePacket(outbound);  // return to pool
      outbound = nullptr;
    } else if (millisHasNowPassed(outbound_expiry)) {
      MESH_DEBUG_PRINTLN("%s MyDispatcher::loop(): WARNING: outbound packed send timed out!", getLogDateTime());

      _radio->onSendFinished();
      logTxFail(outbound);

      releasePacket(outbound);  // return to pool
      outbound = nullptr;
    } else {
      return;  // can't do any more radio activity until send is complete or timed out
    }

    // going back into receive mode now...
    next_agc_reset_time = futureMillis(getAGCResetInterval());
  }

  if (getAGCResetInterval() > 0 && millisHasNowPassed(next_agc_reset_time)) {
    _radio->resetAGC();
    next_agc_reset_time = futureMillis(getAGCResetInterval());
  }

  // check inbound (delayed) queue
  {
    AprsPacket* pkt = _mgr->getNextInbound(_ms->getMillis());
    if (pkt) {
      processRecvPacket(pkt);
    }
  }
  checkRecv();
  checkSend();
}

bool MyAprsDispatcher::tryParsePacket(AprsPacket* pkt, const uint8_t* raw, int len) {
  int i = 0;

  memcpy(pkt->payload, &raw[i], pkt->payload_len);

  return true;  // success
}

void MyAprsDispatcher::checkRecv() {
  AprsPacket* pkt;
  float score;
  uint32_t air_time;
  {
    uint8_t raw[MAX_TRANS_UNIT+1];
    int len = _radio->recvRaw(raw, MAX_TRANS_UNIT);
    if (len > 0) {
      logRxRaw(_radio->getLastSNR(), _radio->getLastRSSI(), raw, len);

      pkt = _mgr->allocNew();
      if (pkt == nullptr) {
        MESH_DEBUG_PRINTLN("%s MyDispatcher::checkRecv(): WARNING: received data, no unused packets available!", getLogDateTime());
      } else {
        if (tryParsePacket(pkt, raw, len)) {
          pkt->snr = _radio->getLastSNR() * 4.0f;
          score = _radio->packetScore(_radio->getLastSNR(), len);
          air_time = _radio->getEstAirtimeFor(len);
          rx_air_time += air_time;
        } else {
          _mgr->free(pkt);  // put back into pool
          pkt = nullptr;
        }
      }
    } else {
      pkt = nullptr;
    }
  }
  if (pkt) {
    #if APRS_PACKET_LOGGING
    Serial.print(getLogDateTime());
    Serial.printf(": RX, len=%d (type=%d, route=%s, payload_len=%d) SNR=%d RSSI=%d score=%d time=%d",
            pkt->size, pkt->packet.type, pkt->packet.path, strlen(pkt->packet.content),
            (int)pkt->snr, (int)_radio->getLastRSSI(), (int)(score*1000), air_time);
#endif
    logRx(pkt, score);   // hook for custom logging

    n_recv++;

    int _delay = calcRxDelay(score, air_time);
    if (_delay < 50) {
      MESH_DEBUG_PRINTLN("%s MyDispatcher::checkRecv(), score delay below threshold (%d)", getLogDateTime(), _delay);
      processRecvPacket(pkt);   // is below the score delay threshold, so process immediately
    } else {
      MESH_DEBUG_PRINTLN("%s MyDispatcher::checkRecv(), score delay is: %d millis", getLogDateTime(), _delay);
      if (_delay > MAX_RX_DELAY_MILLIS) {
        _delay = MAX_RX_DELAY_MILLIS;
      }
      _mgr->queueInbound(pkt, futureMillis(_delay)); // add to delayed inbound queue
    }
  }
}

void MyAprsDispatcher::processRecvPacket(AprsPacket* pkt) {
  mesh::DispatcherAction action = onRecvPacket(pkt);
  if (action == ACTION_RELEASE) {
    _mgr->free(pkt);
  } else if (action == ACTION_MANUAL_HOLD) {
    // sub-class is wanting to manually hold Packet instance, and call releasePacket() at appropriate time
  } else {   // ACTION_RETRANSMIT*
    uint8_t priority = (action >> 24) - 1;
    uint32_t _delay = action & 0xFFFFFF;

    _mgr->queueOutbound(pkt, priority, futureMillis(_delay));
  }
}

void MyAprsDispatcher::checkSend() {
  if (_mgr->getOutboundCount(_ms->getMillis()) == 0) return;
  
  if (!millisHasNowPassed(next_tx_time)) return;
  if (_radio->isReceiving()) {
    if (cad_busy_start == 0) {
      cad_busy_start = _ms->getMillis();   // record when CAD busy state started
    }

    if (_ms->getMillis() - cad_busy_start > getCADFailMaxDuration()) {
      _err_flags |= ERR_EVENT_CAD_TIMEOUT;

      MESH_DEBUG_PRINTLN("%s MyDispatcher::checkSend(): CAD busy max duration reached!", getLogDateTime());
      // channel activity has gone on too long... (Radio might be in a bad state)
      // force the pending transmit below...
    } else {
      next_tx_time = futureMillis(getCADFailRetryDelay());
      return;
    }
  }
  cad_busy_start = 0;  // reset busy state

  outbound = _mgr->getNextOutbound(_ms->getMillis());
  if (outbound) {
    int len = 0;
    uint8_t raw[MAX_TRANS_UNIT];

    raw[len++] = outbound->header;
    if (outbound->hasTransportCodes()) {
      memcpy(&raw[len], &outbound->transport_codes[0], 2); len += 2;
      memcpy(&raw[len], &outbound->transport_codes[1], 2); len += 2;
    }
    raw[len++] = outbound->path_len;
    len += AprsPacket::writePath(&raw[len], outbound->path, outbound->path_len);

    if (len + outbound->payload_len > MAX_TRANS_UNIT) {
      MESH_DEBUG_PRINTLN("%s MyDispatcher::checkSend(): FATAL: Invalid packet queued... too long, len=%d", getLogDateTime(), len + outbound->payload_len);
      _mgr->free(outbound);
      outbound = nullptr;
    } else {
      memcpy(&raw[len], outbound->payload, outbound->payload_len); len += outbound->payload_len;

      uint32_t max_airtime = _radio->getEstAirtimeFor(len)*3/2;
      outbound_start = _ms->getMillis();
      bool success = _radio->startSendRaw(raw, len);
      if (!success) {
        MESH_DEBUG_PRINTLN("%s MyDispatcher::loop(): ERROR: send start failed!", getLogDateTime());

        logTxFail(outbound);
  
        releasePacket(outbound);  // return to pool
        outbound = nullptr;
        return;
      }
      outbound_expiry = futureMillis(max_airtime);

    #if MESH_PACKET_LOGGING
      Serial.print(getLogDateTime());
      Serial.printf(": TX, len=%d (type=%d, route=%s, payload_len=%d)", 
            len, outbound->getPayloadType(), outbound->isRouteDirect() ? "D" : "F", outbound->payload_len);
      if (outbound->getPayloadType() == PAYLOAD_TYPE_PATH || outbound->getPayloadType() == PAYLOAD_TYPE_REQ
        || outbound->getPayloadType() == PAYLOAD_TYPE_RESPONSE || outbound->getPayloadType() == PAYLOAD_TYPE_TXT_MSG) {
        Serial.printf(" [%02X -> %02X]\n", (uint32_t)outbound->payload[1], (uint32_t)outbound->payload[0]);
      } else {
        Serial.printf("\n");
      }
    #endif
    }
  }
}

AprsPacket* MyAprsDispatcher::obtainNewPacket() {
  auto pkt = _mgr->allocNew();  // TODO: zero out all fields
  if (pkt == nullptr) {
    _err_flags |= ERR_EVENT_FULL;
  } else {
    pkt->payload_len = pkt->path_len = 0;
    pkt->snr = 0;
  }
  return pkt;
}

void MyAprsDispatcher::releasePacket(AprsPacket* packet) {
  _mgr->free(packet);
}

void MyAprsDispatcher::sendPacket(AprsPacket* packet, uint8_t priority, uint32_t delay_millis) {
  if (!AprsPacket::isValidPathLen(packet->path_len) || packet->payload_len > MAX_PACKET_PAYLOAD) {
    MESH_DEBUG_PRINTLN("%s MyDispatcher::sendPacket(): ERROR: invalid packet... path_len=%d, payload_len=%d", getLogDateTime(), (uint32_t) packet->path_len, (uint32_t) packet->payload_len);
    _mgr->free(packet);
  } else {
    _mgr->queueOutbound(packet, priority, futureMillis(delay_millis));
  }
}

// Utility function -- handles the case where millis() wraps around back to zero
//   2's complement arithmetic will handle any unsigned subtraction up to HALF the word size (32-bits in this case)
bool MyAprsDispatcher::millisHasNowPassed(unsigned long timestamp) const {
  return (long)(_ms->getMillis() - timestamp) > 0;
}

unsigned long MyAprsDispatcher::futureMillis(int millis_from_now) const {
  return _ms->getMillis() + millis_from_now;
}

const char *MyMesh::getLogDateTime() {
  static char tmp[32];
  uint32_t now = getRTCClock()->getCurrentTime();
  DateTime dt = DateTime(now);
  sprintf(tmp, "%02d:%02d:%02d - %d/%d/%d U", dt.hour(), dt.minute(), dt.second(), dt.day(), dt.month(),
          dt.year());
  return tmp;
}

void MyMesh::logRxRaw(float snr, float rssi, const uint8_t raw[], int len) {
#if MESH_PACKET_LOGGING
  Serial.print(getLogDateTime());
  Serial.print(" RAW: ");
  mesh::Utils::printHex(Serial, raw, len);
  Serial.println();
#endif
}

void MyMesh::logRx(mesh::Packet *pkt, int len, float score) {
#ifdef WITH_BRIDGE
  if (_prefs.bridge_pkt_src == 1) {
    bridge.sendPacket(pkt);
  }
#endif

  if (_logging) {
    File f = openAppend(PACKET_LOG_FILE);
    if (f) {
      f.print(getLogDateTime());
      f.printf(": RX, len=%d (type=%d, route=%s, payload_len=%d) SNR=%d RSSI=%d score=%d", len,
               pkt->getPayloadType(), pkt->isRouteDirect() ? "D" : "F", pkt->payload_len,
               (int)_radio->getLastSNR(), (int)_radio->getLastRSSI(), (int)(score * 1000));

      if (pkt->getPayloadType() == PAYLOAD_TYPE_PATH || pkt->getPayloadType() == PAYLOAD_TYPE_REQ ||
          pkt->getPayloadType() == PAYLOAD_TYPE_RESPONSE || pkt->getPayloadType() == PAYLOAD_TYPE_TXT_MSG) {
        f.printf(" [%02X -> %02X]\n", (uint32_t)pkt->payload[1], (uint32_t)pkt->payload[0]);
      } else {
        f.printf("\n");
      }
      f.close();
    }
  }
}

void MyMesh::logTx(mesh::Packet *pkt, int len) {
#ifdef WITH_BRIDGE
  if (_prefs.bridge_pkt_src == 0) {
    bridge.sendPacket(pkt);
  }
#endif

  if (_logging) {
    File f = openAppend(PACKET_LOG_FILE);
    if (f) {
      f.print(getLogDateTime());
      f.printf(": TX, len=%d (type=%d, route=%s, payload_len=%d)", len, pkt->getPayloadType(),
               pkt->isRouteDirect() ? "D" : "F", pkt->payload_len);

      if (pkt->getPayloadType() == PAYLOAD_TYPE_PATH || pkt->getPayloadType() == PAYLOAD_TYPE_REQ ||
          pkt->getPayloadType() == PAYLOAD_TYPE_RESPONSE || pkt->getPayloadType() == PAYLOAD_TYPE_TXT_MSG) {
        f.printf(" [%02X -> %02X]\n", (uint32_t)pkt->payload[1], (uint32_t)pkt->payload[0]);
      } else {
        f.printf("\n");
      }
      f.close();
    }
  }
}

void MyMesh::logTxFail(mesh::Packet *pkt, int len) {
  if (_logging) {
    File f = openAppend(PACKET_LOG_FILE);
    if (f) {
      f.print(getLogDateTime());
      f.printf(": TX FAIL!, len=%d (type=%d, route=%s, payload_len=%d)\n", len, pkt->getPayloadType(),
               pkt->isRouteDirect() ? "D" : "F", pkt->payload_len);
      f.close();
    }
  }
}

}
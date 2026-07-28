#pragma once

#include "aprs/AprsRadioHw.h"

// ============================================================================
// AprsRadioHwSim — implémentation native d'IAprsRadioHw (cf. aprs/AprsRadioHw.h) :
// pas de SX1262 réel, la bascule LoRa/FSK est purement logique
// (SimWorld::fsk_mode, informatif pour le dashboard) et la réception WH65B
// dépile SimWorld::fsk_rx_queue (remplie par "sim rx fsk <hex>" ou
// POST /api/rx). Contrepartie réelle : variant/AprsRadioHwReal.h.
// ============================================================================

class AprsRadioHwSim : public IAprsRadioHw {
public:
  bool switchToFsk() override;
  bool switchToLora() override;
  bool receiveWh65bFrame(uint32_t timeoutMs, uint8_t* outBuf, float* outRssi) override;
  bool relayWh65bFrame(const uint8_t* data, size_t len, int8_t powerDbm) override;
  void standby() override {}
};

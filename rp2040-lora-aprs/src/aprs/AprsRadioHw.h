#pragma once

#include <stdint.h>
#include <stddef.h>

class IAprsRadioHw {
public:
  virtual ~IAprsRadioHw() = default;

  virtual bool switchToFsk() = 0;
  virtual bool switchToLora() = 0;

  // WH65B_PAYLOAD_LEN octets attendus (cf. LoRa433RadioMode.h)
  virtual bool receiveWh65bFrame(uint32_t timeoutMs, uint8_t* outBuf, float* outRssi) = 0;

  // Relais brut du protocole WH65B (pas une conversion APRS)
  virtual bool relayWh65bFrame(const uint8_t* data, size_t len, int8_t powerDbm) = 0;

  virtual void standby() = 0;
};

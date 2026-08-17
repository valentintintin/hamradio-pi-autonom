#pragma once

#include <Aprs.h>
#include <stdint.h>

class AprsEventCallback {
public:
  virtual ~AprsEventCallback() = default;

  virtual void onAprsFrameReceived(const aprs::PacketLite& pkt, float rssi, float snr) {}
  virtual void onAprsMessageReceived(const char* from, const char* message) {}
  virtual void fillTelemetryData(aprs::Telemetry& telemetry) {}
  virtual void fillWeatherData(aprs::Weather& weather) {}
  virtual void fillStatusText(char* buf, size_t len) { if (len) buf[0] = '\0'; }
};

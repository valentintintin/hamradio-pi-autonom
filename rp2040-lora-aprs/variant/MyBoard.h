#pragma once

#include <Arduino.h>
#include <MeshCore.h>

class MyBoard : public mesh::MainBoard {
protected:
  uint8_t startup_reason;

public:
  void begin();
  uint8_t getStartupReason() const override { return startup_reason; }

  uint16_t getBattMilliVolts() override {
    return 0;
  }

  const char* getManufacturerName() const override { return "F4HVV soft & F4ISE hard"; }

  void reboot() override { rp2040.reboot(); }

  bool startOTAUpdate(const char* id, char reply[]) override { return false; }
};

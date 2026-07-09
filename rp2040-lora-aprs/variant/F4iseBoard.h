#pragma once

#include <Arduino.h>
#include <MeshCore.h>

// ============================================================================
// Board RP-LoRA_Mini_v3_dual (F4ISE)
// Pico W + 2x SX1262 (868 + 433)
// ============================================================================

#define PIN_VBAT_READ       28   // partagé avec APRS_RXEN — si ADC batterie nécessaire,
                                 // utiliser un mux ou lire quand radio idle
#define BATTERY_SAMPLES     8
#define ADC_MULTIPLIER      (3.0f * 3.3f * 1000)

class F4iseBoard : public mesh::MainBoard {
protected:
  uint8_t startup_reason;

public:
  void begin();
  uint8_t getStartupReason() const override { return startup_reason; }

  uint16_t getBattMilliVolts() override {
    // TODO: implémenter si diviseur de tension câblé
    return 0;
  }

  const char* getManufacturerName() const override { return "F4ISE RP-LoRA Mini v3"; }

  void reboot() override { rp2040.reboot(); }

  bool startOTAUpdate(const char* id, char reply[]) override { return false; }
};

#pragma once

#include <MeshCore.h>
#include <Arduino.h>
#include <helpers/RP2040OTA.h>

#if defined(ARDUINO_ARCH_RP2040)
  #include <hardware/clocks.h>
#endif

// built-ins
#define  PIN_VBAT_READ    26
#define  ADC_MULTIPLIER   (3.1 * 3.3 * 1000) // MT Uses 3.1
#define  PIN_LED_BUILTIN  LED_BUILTIN

class PicoWBoard : public mesh::MainBoard {
protected:
  uint8_t startup_reason;
  RP2040OTAController ota;

public:
  void begin();
  uint8_t getStartupReason() const override { return startup_reason; }

  void onBeforeTransmit() override {
    digitalWrite(LED_BUILTIN, HIGH);   // turn TX LED on
  }

  void onAfterTransmit() override {
    digitalWrite(LED_BUILTIN, LOW);   // turn TX LED off
  }

  #define BATTERY_SAMPLES 8

  uint16_t getBattMilliVolts() override {
    analogReadResolution(12);

    uint32_t raw = 0;
    for (int i = 0; i < BATTERY_SAMPLES; i++) {
      raw += analogRead(PIN_VBAT_READ);
    }
    raw = raw / BATTERY_SAMPLES;

    return (ADC_MULTIPLIER * raw) / 4096;
  }

  const char* getManufacturerName() const override {
    return "RP Solar OTA";
  }

  void reboot() override {
    //NVIC_SystemReset();
    rp2040.reboot();
  }

  bool startOTAUpdate(const char *id, char reply[]) override;
  bool handleOTACommand(const char *command, char reply[]) override;
  bool handleOTABinaryCommand(uint8_t opcode, const uint8_t *payload, size_t payload_len, char reply[]) override;
};

#if defined(ARDUINO_ARCH_RP2040)
#ifndef RP2040_ACTIVE_CLOCK_MHZ
  #ifdef RP2040_CPU_FREQ_MHZ
    #define RP2040_ACTIVE_CLOCK_MHZ RP2040_CPU_FREQ_MHZ
  #else
    #define RP2040_ACTIVE_CLOCK_MHZ 125
  #endif
#endif

#ifndef RP2040_OTA_CLOCK_MHZ
  #define RP2040_OTA_CLOCK_MHZ 125
#endif

inline void rp2040_apply_clock_profile(uint32_t clock_mhz) {
  set_sys_clock_khz(clock_mhz * 1000, true);
}

inline void rp2040_restore_active_profile() {
  rp2040_apply_clock_profile(RP2040_ACTIVE_CLOCK_MHZ);
}

inline void rp2040_enter_ota_profile() {
  rp2040_apply_clock_profile(RP2040_OTA_CLOCK_MHZ);
}
#endif

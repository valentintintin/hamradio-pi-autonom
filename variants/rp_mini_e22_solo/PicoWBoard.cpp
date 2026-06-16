#include <Arduino.h>
#include "PicoWBoard.h"

//#include <bluefruit.h>
#include <Wire.h>

//static BLEDfu bledfu;

#if defined(ARDUINO_ARCH_RP2040)
  //rp2040_restore_active_profile();
#endif

static void connect_callback(uint16_t conn_handle) {
  (void)conn_handle;
  MESH_DEBUG_PRINTLN("BLE client connected");
}

static void disconnect_callback(uint16_t conn_handle, uint8_t reason) {
  (void)conn_handle;
  (void)reason;

  MESH_DEBUG_PRINTLN("BLE client disconnected");
}

void PicoWBoard::begin() {
  // for future use, sub-classes SHOULD call this from their begin()
  startup_reason = BD_STARTUP_NORMAL;
  pinMode(PIN_VBAT_READ, INPUT);
#ifdef PIN_USER_BTN
  pinMode(PIN_USER_BTN, INPUT_PULLUP);
#endif

#if defined(PIN_BOARD_SDA) && defined(PIN_BOARD_SCL)
  //Wire.setPins(PIN_BOARD_SDA, PIN_BOARD_SCL);
  Wire.setSDA(PIN_BOARD_SDA);
  Wire.setSCL(PIN_BOARD_SCL);
#endif

  Wire.begin();

  //pinMode(SX126X_POWER_EN, OUTPUT);
  //digitalWrite(SX126X_POWER_EN, HIGH);
  delay(10);   // give sx1262 some time to power up
}

bool PicoWBoard::startOTAUpdate(const char *id, char reply[]) {
#if defined(ARDUINO_ARCH_RP2040)
  rp2040_enter_ota_profile();
#endif
  return ota.startSession(id, reply);
}

bool PicoWBoard::handleOTACommand(const char *command, char reply[]) {
#if defined(ARDUINO_ARCH_RP2040)
  rp2040_enter_ota_profile();
#endif
  bool ok = ota.handleCommand(command, reply);
#if defined(ARDUINO_ARCH_RP2040)
  if (!ota.isSleepInhibited()) {
    rp2040_restore_active_profile();
  }
#endif
  return ok;
}

bool PicoWBoard::handleOTABinaryCommand(uint8_t opcode, const uint8_t *payload, size_t payload_len, char reply[]) {
#if defined(ARDUINO_ARCH_RP2040)
  rp2040_enter_ota_profile();
#endif
  bool ok = ota.handleBinaryCommand(opcode, payload, payload_len, reply);
#if defined(ARDUINO_ARCH_RP2040)
  if (!ota.isSleepInhibited()) {
    rp2040_restore_active_profile();
  }
#endif
  return ok;
}

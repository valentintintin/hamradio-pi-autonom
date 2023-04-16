#include <Arduino.h>
#include <radio/radio.h>
#include <ArduinoLog.h>

#include "../lib/mpptChg/mpptChg.h"
#include "../lib/Aprs/Aprs.h"
#include "System.h"
#include "Communication.h"
#include "MpptMonitor.h"

System systemControl;
Communication communication(&systemControl);
MpptMonitor mpptMonitor(&communication);
RadioEvents_t RadioEvents;

void sentEvent() {
    communication.sent();
}

void receivedEvent(uint8_t * payload, uint16_t size, int16_t rssi, int8_t snr) {
    communication.received(payload, size, rssi, snr);
}

void userKey() {
    Log.noticeln(F("User key pressed"));

    delay(100);

    systemControl.turnScreenOn();
}

void setup() {
    RadioEvents.RxDone = receivedEvent;
    RadioEvents.TxDone = sentEvent;

    boardInitMcu();

    Serial.begin(115200);
    Log.begin(LOG_LEVEL_VERBOSE, &Serial);
    Log.infoln(F("Starting"));

    systemControl.begin();
    attachInterrupt(USER_BUTTON, userKey, FALLING);

    if (!communication.begin(&RadioEvents)) {
        systemControl.displayText(PSTR("LoRa error"), PSTR("Init failed"), 5000);
    }

    if (!mpptMonitor.begin()) {
        systemControl.displayText(PSTR("Mppt error"), PSTR("Init failed"), 5000);
    }

    systemControl.turnOffRGB();

    systemControl.displayText(PSTR("Init"), PSTR("Ready"));

    Log.infoln(F("Started"));

    communication.sendPosition(PSTR("Ready"));
}

void loop() {
    if (Serial.available()) {
        Log.traceln(F("Serial incoming"));
        const char* stringReceived = Serial.readString().c_str();
        Log.infoln(F("Serial received %s"), stringReceived);
    }

    mpptMonitor.update();
    communication.update();
    systemControl.update();
}
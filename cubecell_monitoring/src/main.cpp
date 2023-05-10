#include <Arduino.h>
#include <radio/radio.h>
#include <ArduinoLog.h>
#include "System.h"

Logging Log2 = Logging();

SH1107Wire  display(0x3c, 500000, SDA, SCL ,GEOMETRY_128_64,GPIO10); // addr, freq, sda, scl, resolution, rst
CubeCell_NeoPixel pixels = CubeCell_NeoPixel(1, RGB, NEO_GRB + NEO_KHZ800);
System systemControl(&display, &pixels);
Communication communication(&systemControl);

RadioEvents_t radioEvents;

void sentEvent() {
    communication.sent();
}

void receivedEvent(uint8_t * payload, uint16_t size, int16_t rssi, int8_t snr) {
    communication.received(payload, size, rssi, snr);
}

void userButton() {
    Log.traceln(F("[GPIO] User key pressed"));

    delay(250);

    systemControl.userButton();
}

void setup() {
    radioEvents.RxDone = receivedEvent;
    radioEvents.TxDone = sentEvent;

    boardInitMcu();

    pinMode(Vext, OUTPUT);
    digitalWrite(Vext, HIGH); // 0V

    Serial.begin(115200);
    Serial1.begin(115200);

    Log.begin(LOG_LEVEL_INFO, &Serial);
    Log2.begin(LOG_LEVEL_INFO, &Serial1);

    systemControl.begin(&radioEvents);

    pinMode(USER_KEY, INPUT);
    attachInterrupt(USER_KEY, userButton, FALLING);

//    systemControl.communication->sendPosition(PSTR("Ready"));
//    systemControl.communication->sendMessage(PSTR(APRS_DESTINATION), PSTR("Ping"));
}

void loop() {
    systemControl.update();
}
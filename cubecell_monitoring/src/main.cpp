#include <Arduino.h>
#include <radio/radio.h>
#include <ArduinoLog.h>
#include <innerWdt.h>
#include "System.h"

extern JsonWriter serialJsonWriter(&SerialPiUsed);

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

void setup() {
    radioEvents.RxDone = receivedEvent;
    radioEvents.TxDone = sentEvent;

    boardInitMcu();

    pinMode(Vext, OUTPUT);
    digitalWrite(Vext, HIGH); // 0V

    Serial.begin(115200);

    if (&SerialPiUsed != &Serial) {
        SerialPiUsed.begin(115200);
    }

    Log.begin(LOG_LEVEL , &Serial);

//    innerWdtEnable(true);

    systemControl.begin(&radioEvents);
}

void loop() {
    systemControl.update();
}
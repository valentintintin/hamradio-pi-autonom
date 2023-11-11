#include <Arduino.h>
#include <radio/radio.h>
#include <ArduinoLog.h>
#include "System.h"

#ifdef USE_SOFT_SERIAL
#include <softSerial.h>
extern softSerial *SerialPi = new softSerial(PIN_PI_TX, PIN_PI_RX);
#else
extern HardwareSerial *SerialPi = &Serial1;
#endif
extern JsonWriter serialJsonWriter(SerialPi);
extern char bufferText[255]{};

RadioEvents_t radioEvents;
TimerEvent_t wakeUpEvent;

CubeCell_NeoPixel pixels = CubeCell_NeoPixel(1, RGB, NEO_GRB + NEO_KHZ800);
#ifdef USE_SCREEN
SH1107Wire display(0x3c, 500000, SDA, SCL, GEOMETRY_128_64, GPIO10); // addr, freq, sda, scl, resolution, rst
System systemControl(&display, &pixels, &wakeUpEvent);
#else
System systemControl(&pixels, &wakeUpEvent);
#endif
Communication communication(&systemControl);

void sentEvent() {
    communication.sent();
}

void receivedEvent(uint8_t * payload, uint16_t size, int16_t rssi, int8_t snr) {
    communication.received(payload, size, rssi, snr);
}

void onWakeUp() {
    CySoftwareReset();
}

void setup() {
    radioEvents.RxDone = receivedEvent;
    radioEvents.TxDone = sentEvent;
    TimerInit(&wakeUpEvent, onWakeUp);

    boardInitMcu();

    pinMode(Vext, OUTPUT);
    digitalWrite(Vext, LOW); // 3.3V

    Serial.begin(115200);
#ifdef USE_SOFT_SERIAL
    SerialPi->begin(38400);
#else
    SerialPi->begin(115200);
#endif

    Log.begin(LOG_LEVEL, &Serial);

    systemControl.begin(&radioEvents);

#ifndef USE_SCREEN
    // To catch Serial in dev
    delay(2000);
#endif
}

void loop() {
    systemControl.update();
}
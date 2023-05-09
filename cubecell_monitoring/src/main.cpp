#include <Arduino.h>
#include <radio/radio.h>
#include <ArduinoLog.h>
#include "System.h"

Logging Log2 = Logging();

SH1107Wire display = SH1107Wire(0x3c, 500000, SDA, SCL, GEOMETRY_128_64, GPIO10);
CubeCell_NeoPixel pixels = CubeCell_NeoPixel(1, RGB, NEO_GRB + NEO_KHZ800);
System systemControl(&display, &pixels);
Communication communication(&systemControl);

RadioEvents_t radioEvents;

static TimerEvent_t sleep;
static TimerEvent_t wakeUp;
bool isLowPower = false;

void sentEvent() {
    communication.sent();
}

void receivedEvent(uint8_t * payload, uint16_t size, int16_t rssi, int8_t snr) {
    communication.received(payload, size, rssi, snr);
}

void onSleep() {
    isLowPower = true;

    systemControl.sleep();

//    TimerSetValue(&wakeUp, TIME_SLEEP_MS);
//    TimerStart(&wakeUp);
}

void onWakeUp() {
    isLowPower = false;

    systemControl.wakeUp();

//    TimerSetValue(&sleep, TIME_WAKEUP_MS);
//    TimerStart(&sleep);
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
    digitalWrite(Vext, HIGH);

    Serial.begin(115200);
    Serial1.begin(115200);

    Log.begin(LOG_LEVEL_INFO, &Serial);
    Log2.begin(LOG_LEVEL_INFO, &Serial1);

    systemControl.begin(&radioEvents);

//    if (TIME_SLEEP_MS == 0) {
        pinMode(USER_KEY, INPUT);
        attachInterrupt(USER_KEY, userButton, FALLING);
//    }

//    systemControl.communication->sendPosition(PSTR("Ready"));
//    systemControl.communication->sendMessage(PSTR(APRS_DESTINATION), PSTR("Ping"));

//    if (TIME_SLEEP_MS > 0) {
//        TimerInit(&sleep, onSleep);
//        TimerInit(&wakeUp, onWakeUp);
//        onSleep();
//    }
}

void loop() {
    systemControl.update();

    if (isLowPower){
        lowPowerHandler();
    }
}
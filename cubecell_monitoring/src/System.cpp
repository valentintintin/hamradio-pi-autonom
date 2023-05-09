#include <radio/radio.h>
#include "System.h"
#include "ArduinoLog.h"

System::System(SH1107Wire *display, CubeCell_NeoPixel *pixels) :
display(display), pixels(pixels),
mpptMonitor(this), weatherSensors(this), gpio(this), command(this) {
}

bool System::begin(RadioEvents_t *radioEvents) {
    Log.infoln(F("[SYSTEM] Starting"));
    Log2.infoln(F("[SYSTEM] Starting"));

    pixels->begin();
    pixels->clear();

    if (!display->init()) {
        Log.warningln(F("Display error"));
    }

    display->flipScreenVertically();

    if (USE_SCREEN_AND_LED) {
        turnScreenOn();
    } else {
        turnScreenOff();
    }

    weatherSensors.begin();
    mpptMonitor.begin();
    communication->begin(radioEvents);

//    RTC.setClockMode(false);  // set to 24h
//    RTC.setYear(23);
//    RTC.setMonth(5);
//    RTC.setDate(7);
//    RTC.setDoW(7);
//    RTC.setHour(8);
//    RTC.setMinute(39);
//    RTC.setSecond(30);

    nowToString(bufferText);

    Log.infoln(F("[SYSTEM] Started at %s"), bufferText);
    Log2.infoln(F("[SYSTEM] Started DateTime:%s"), bufferText);
    displayText(PSTR("Started"), bufferText);

    return true;
}

void System::update() {
    Stream *streamReceived = nullptr;

    if (Serial1.available()) {
        streamReceived = &Serial1;
        Log.verboseln(F("Serial 2 incoming"));
    } else if (Serial.available()) {
        streamReceived = &Serial;
        Log.verboseln(F("Serial 1 (USB) incoming"));
    }

    if (streamReceived != nullptr) {
        turnOnRGB(COLOR_RXWINDOW1);

        size_t lineLength = streamReceived->readBytesUntil('\n', bufferText, 127);
        bufferText[lineLength] = '\0';

        command.processCommand(bufferText);
    } else {
        mpptMonitor.update();
        weatherSensors.update();

        if (forceSendTelemetry || timerTime.hasExpired()) {
            timeUpdate();
            timerTime.restart();
        }

        if (timerTelemetry.hasExpired() || timerPosition.hasExpired() || forceSendTelemetry) {
            communication->update(forceSendTelemetry || timerTelemetry.hasExpired(), forceSendTelemetry || timerPosition.hasExpired());
            forceSendTelemetry = false;

            if (timerTelemetry.hasExpired()) {
                timerTelemetry.restart();
            }

            if (timerPosition.hasExpired()) {
                timerPosition.restart();
            }
        }
    }

    delay(10);
}

void System::turnOnRGB(uint32_t color) {
    if (screenOn) {
        Log.verboseln(F("Turn led color : %l"), color);

        uint8_t red, green, blue;
        red = (uint8_t) (color >> 16);
        green = (uint8_t) (color >> 8);
        blue = (uint8_t) color;
        pixels->setPixelColor(0, CubeCell_NeoPixel::Color(red, green, blue));
        pixels->show();
    }
}

void System::turnOffRGB() {
    turnOnRGB(0);
    digitalWrite(Vext, HIGH);
}

void System::turnScreenOn() {
    if (!screenOn) {
        Log.verboseln(F("Turn screen on"));

        display->wakeup();
        screenOn = true;
    }
}

void System::turnScreenOff() {
    if (screenOn) {
        Log.verboseln(F("Turn screen off"));

        display->sleep();
        screenOn = false;
    }

    turnOffRGB();
}

void System::displayText(const char *title, const char *content, uint16_t pause) const {
    if (!screenOn) {
        return;
    }

    Log.traceln(F("Display : %s --> %s"), title, content);

    display->clear();
    display->drawString(0, 0, title);
    display->drawStringMaxWidth(0, 10, 120, content);
    display->display();
    delay(pause);
}

void System::userButton() {
    if (millis() >= 5000) {
        forceSendTelemetry = true;
    }
}

void System::wakeUp() {
    Radio.Rx(0);
}

void System::sleep() {
    turnScreenOff();
    Radio.Sleep();
}

void System::nowToString(char *result) {
    DateTime now = RTClib::now();
    sprintf(result, "%04d-%02d-%02dT%02d:%02d:%02d %ld %ld", now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second(), now.unixtime(), millis() / 1000);
}

void System::timeUpdate() {
    nowToString(bufferText);
    Log.infoln(F("[TIME] %s"), bufferText);
    Log2.infoln(F("[TIME] %s"), bufferText);
    displayText(PSTR("Time"), bufferText, 1000);
}

#include <radio/radio.h>
#include "System.h"
#include "ArduinoLog.h"

System::System(SH1107Wire *display, CubeCell_NeoPixel *pixels) :
        display(display), pixels(pixels), RTC(WireUsed),
        mpptMonitor(this, WireUsed), weatherSensors(this), gpio(this), command(this) {
}

bool System::begin(RadioEvents_t *radioEvents) {
    bool boxOpened = isBoxOpened();

    Log.infoln(F("[SYSTEM] Starting"));
    serialJsonWriter
            .beginObject()
            .property(F("type"), PSTR("system"))
            .property(F("state"), PSTR("starting"))
            .property(F("boxOpened"), boxOpened)
            .endObject(); SerialPiUsed.println();

    Wire.begin(SDA, SCL, 500000);

    if (&WireUsed == &Wire1) {
        Wire1.begin(SDA1, SCL1);
    }

    pixels->begin();
    pixels->clear();

    if (!display->init()) {
        serialError(PSTR("[SYSTEM] Display error"));
    }

    turnScreenOn();

    weatherSensors.begin();
    mpptMonitor.begin();
    communication->begin(radioEvents);

    if (USE_RTC) {
        if (SET_RTC > 0) {
            Log.warningln(F("[TIME] Set clock to %l"), SET_RTC + 60);
            RTC.setEpoch(SET_RTC + 60);
        }

        setTimeFromRTcToInternalRtc(RTClib::now().unixtime());

        nowToString(bufferText);
        Log.infoln(F("[SYSTEM] Started at %s"), bufferText);
    } else {
        Log.infoln(F("[SYSTEM] Started"));
    }

    serialJsonWriter
            .beginObject()
            .property(F("type"), PSTR("system"))
            .property(F("state"), PSTR("started"))
            .property(F("boxOpened"), boxOpened)
            .endObject(); SerialPiUsed.println();
    displayText(PSTR("Started"), bufferText);

    return true;
}

void System::update() {
    Stream *streamReceived = nullptr;

    if (Serial1.available()) {
        streamReceived = &Serial1;
        Log.verboseln(F("Serial Pi incoming"));
    } else if (Serial.available()) {
        streamReceived = &Serial;
        Log.verboseln(F("Serial USB incoming"));
    }

    if (streamReceived != nullptr) {
        turnOnRGB(COLOR_RXWINDOW1);

        size_t lineLength = streamReceived->readBytesUntil('\n', bufferText, 127);
        bufferText[lineLength] = '\0';

        command.processCommand(bufferText);
    } else {
        if (timerSecond.hasExpired()) {
            if (timerBoxOpened.hasExpired() && isBoxOpened()) {
                Log.warningln(F("[SYSTEM] Box opened !"));
                serialJsonWriter
                        .beginObject()
                        .property(F("type"), PSTR("system"))
                        .property(F("state"), PSTR("alert"))
                        .property(F("boxOpened"), true)
                        .endObject(); SerialPiUsed.println();

                timerBoxOpened.restart();
                communication->sendMessage(PSTR(APRS_DESTINATION), PSTR("Box opened !"));
            }
        }

        if (forceSendTelemetry || timerTime.hasExpired()) {
            showTime();

            gpio.printJson();

            serialJsonWriter
                    .beginObject()
                    .property(F("type"), PSTR("system"))
                    .property(F("state"), PSTR("running"))
                    .property(F("boxOpened"), isBoxOpened())
                    .endObject(); SerialPiUsed.println();

            timerTime.restart();
        }

        mpptMonitor.update();
        weatherSensors.update();

        if (!mpptMonitor.isPowerEnabled()) {
            if (gpio.isNprEnabled()) {
                gpio.setNpr(false);
            }

            if (gpio.isWifiEnabled()) {
                gpio.setWifi(false);
            }
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

    if (timerScreen.hasExpired()) {
        turnScreenOff();
    }

    if (timerSecond.hasExpired()) {
        timerSecond.restart();
    }

    delay(10);
}

void System::turnOnRGB(uint32_t color) {
    if (screenOn || color == 0) {
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
}

void System::turnScreenOn() {
    if (!screenOn) {
        Log.verboseln(F("Turn screen on"));

        display->wakeup();
        screenOn = true;

        timerScreen.restart();
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
        if (timerScreen.hasExpired()) {
            turnScreenOn();
        } else {
            forceSendTelemetry = true;
        }
    }
}

DateTime System::nowToString(char *result) {
//    DateTime now = RTClib::now();
    DateTime now = DateTime(TimerGetSysTime().Seconds);
    sprintf(result, "%04d-%02d-%02dT%02d:%02d:%02d Uptime %lds", now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second(), millis() / 1000);
    return now;
}

void System::showTime() {
    DateTime now = nowToString(bufferText);

    if (now.year() != 2023) {
        return;
    }

    serialJsonWriter
            .beginObject()
            .property(F("type"), PSTR("time"))
            .property(F("state"), now.unixtime())
            .property(F("uptime"), millis() / 1000)
            .endObject(); SerialPiUsed.println();

    Log.infoln(F("[TIME] %s"), bufferText);
    displayText(PSTR("Time"), bufferText, 1000);
}

void System::setTimeFromRTcToInternalRtc(uint64_t epoch) {
    TimerSysTime_t currentTime;
    currentTime.Seconds = epoch;
    TimerSetSysTime(currentTime);
}

bool System::isBoxOpened() const {
    return millis() >= 60000 && gpio.getLdr() >= LDR_ALARM_LEVEL;
}

void System::serialError(const char *content) const {
    Log.errorln(content);
    serialJsonWriter
            .beginObject()
            .property(F("type"), PSTR("system"))
            .property(F("state"), content)
            .property(F("boxOpened"), isBoxOpened())
            .endObject(); SerialPiUsed.println();
}

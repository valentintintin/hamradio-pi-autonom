#include <radio/radio.h>
#include "ArduinoLog.h"
#include <EEPROM.h>
#include "System.h"

System::System(SH1107Wire *display, CubeCell_NeoPixel *pixels) :
        display(display), pixels(pixels), RTC(WireUsed),
        mpptMonitor(this, WireUsed), weatherSensors(this), gpio(this), command(this) {
}

bool System::begin(RadioEvents_t *radioEvents) {
    Log.infoln(F("[SYSTEM] Starting"));
    printJsonSystem(PSTR("Starting"));

    Wire.begin(SDA, SCL, 500000);

    if (&WireUsed != &Wire) {
        WireUsed.begin(SDA1, SCL1);
    }

    pixels->begin();
    pixels->clear();

    if (!display->init()) {
        serialError(PSTR("[SYSTEM] Display error"));
    }

    turnScreenOn();

    EEPROM.begin(512);

    if (EEPROM.read(EEPROM_ADDRESS_VERSION) != EEPROM_VERSION) {
        setFunctionAllowed(EEPROM_ADDRESS_WATCHDOG_SAFETY, functionsAllowed[EEPROM_ADDRESS_WATCHDOG_SAFETY]);
        setFunctionAllowed(EEPROM_ADDRESS_APRS_DIGIPEATER, functionsAllowed[EEPROM_ADDRESS_APRS_DIGIPEATER]);
        setFunctionAllowed(EEPROM_ADDRESS_APRS_TELEMETRY, functionsAllowed[EEPROM_ADDRESS_APRS_TELEMETRY]);
        setFunctionAllowed(EEPROM_ADDRESS_APRS_POSITION, functionsAllowed[EEPROM_ADDRESS_APRS_POSITION]);

        EEPROM.write(EEPROM_ADDRESS_VERSION, EEPROM_VERSION);
        EEPROM.commit();
    } else {
        functionsAllowed[EEPROM_ADDRESS_WATCHDOG_SAFETY] = EEPROM.read(EEPROM_ADDRESS_WATCHDOG_SAFETY);
        functionsAllowed[EEPROM_ADDRESS_APRS_DIGIPEATER] = EEPROM.read(EEPROM_ADDRESS_APRS_DIGIPEATER);
        functionsAllowed[EEPROM_ADDRESS_APRS_TELEMETRY] = EEPROM.read(EEPROM_ADDRESS_APRS_TELEMETRY);
        functionsAllowed[EEPROM_ADDRESS_APRS_POSITION] = EEPROM.read(EEPROM_ADDRESS_APRS_POSITION);
    }

    sprintf_P(bufferText, PSTR("Watchdog safety: %d Aprs Digi : %d Telem : %d Position : %d"),
            functionsAllowed[EEPROM_ADDRESS_WATCHDOG_SAFETY],
            functionsAllowed[EEPROM_ADDRESS_APRS_DIGIPEATER],
            functionsAllowed[EEPROM_ADDRESS_APRS_TELEMETRY],
            functionsAllowed[EEPROM_ADDRESS_APRS_POSITION]
    );

    Log.infoln(F("[SYSTEM] EEPROM %s"), bufferText);
    displayText(PSTR("EEPROM"), bufferText, 3000);

    weatherSensors.begin();
    mpptMonitor.begin();
    communication->begin(radioEvents);

    if (USE_RTC) {
        if (SET_RTC > 0 && RTClib::now().year() < 2023) {
            Log.warningln(F("[TIME] Set clock to %l"), SET_RTC + 60);
            RTC.setEpoch(SET_RTC + 60);
        }

        setTimeFromRTcToInternalRtc(RTClib::now().unixtime());

        nowToString(bufferText);
        Log.infoln(F("[SYSTEM] Started at %s"), bufferText);
    } else {
        Log.infoln(F("[SYSTEM] Started"));
    }

    printJsonSystem(PSTR("started"));
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

        size_t lineLength = streamReceived->readBytesUntil('\n', bufferText, 150);
        bufferText[lineLength] = '\0';

        Log.traceln(PSTR("[SERIAL] Received %s"), bufferText);

        command.processCommand(bufferText);

        streamReceived->flush();
    } else {
        if (timerSecond.hasExpired()) {
            if (timerBoxOpened.hasExpired() && isBoxOpened()) {
                Log.warningln(F("[SYSTEM] Box opened !"));
                printJsonSystem(PSTR("alert"));

                timerBoxOpened.restart();
                communication->sendMessage(PSTR(APRS_DESTINATION), PSTR("Box opened !"));
            }
        }

        if (forceSendTelemetry || timerTime.hasExpired()) {
            showTime();

            gpio.printJson();

            printJsonSystem(PSTR("running"));

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

        if (timerTelemetry.hasExpired() || timerPosition.hasExpired() || forceSendTelemetry || forceSendPosition) {
            communication->update(forceSendTelemetry || timerTelemetry.hasExpired(), forceSendPosition || timerPosition.hasExpired());
            forceSendTelemetry = false;
            forceSendPosition = false;

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
        if (color == ledColor) {
            return;
        }

        ledColor = color;

        Log.traceln(F("Turn led color : %l"), color);

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
        Log.traceln(F("Turn screen on"));

        display->wakeup();
        screenOn = true;

        timerScreen.restart();
    }
}

void System::turnScreenOff() {
    if (screenOn) {
        Log.traceln(F("Turn screen off"));

        display->sleep();
        screenOn = false;
    }

    turnOffRGB();
}

void System::displayText(const char *title, const char *content, uint16_t pause) const {
    if (!screenOn) {
        return;
    }

    Log.traceln(F("[Display] %s --> %s"), title, content);

    display->clear();
    display->drawString(0, 0, title);
    display->drawStringMaxWidth(0, 10, 120, content);
    display->display();
    delay(pause);
}

DateTime System::nowToString(char *result) {
    DateTime now = RTClib::now();
    if (now.year() < 2023) {
        Log.warningln("[TIME] Use System instead of RTC");
        now = DateTime(TimerGetSysTime().Seconds);
    }
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
    printJsonSystem((char*)content);
}

void System::setFunctionAllowed(byte function, bool allowed) {
    Log.infoln(F("[EEPROM] Set %d to %d"), function, allowed);

    functionsAllowed[function] = allowed;

    EEPROM.write(function, allowed);
    EEPROM.commit();
}

void System::printJsonSystem(const char *state) const {
    serialJsonWriter
            .beginObject()
            .property(F("type"), PSTR("system"))
            .property(F("state"), (char*)state)
            .property(F("boxOpened"), isBoxOpened())
            .property(F("watchdogSefaty"), functionsAllowed[EEPROM_ADDRESS_WATCHDOG_SAFETY])
            .property(F("aprsDigipeater"), functionsAllowed[EEPROM_ADDRESS_APRS_DIGIPEATER])
            .property(F("aprsTelemetry"), functionsAllowed[EEPROM_ADDRESS_APRS_TELEMETRY])
            .property(F("aprsPosition"), functionsAllowed[EEPROM_ADDRESS_APRS_POSITION])
            .endObject(); SerialPiUsed.println();
}

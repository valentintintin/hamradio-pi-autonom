#include <radio/radio.h>
#include "ArduinoLog.h"
#include <EEPROM.h>
#include "System.h"

#ifdef USE_SCREEN
System::System(SH1107Wire *display, CubeCell_NeoPixel *pixels, TimerEvent_t *wakeUpEvent) :
        display(display), pixels(pixels), wakeUpEvent(wakeUpEvent),
        mpptMonitor(this, WireUsed), weatherSensors(this), gpio(this), command(this) {
}
#else
System::System(CubeCell_NeoPixel *pixels, TimerEvent_t *wakeUpEvent) :
        pixels(pixels), RTC(Wire), wakeUpEvent(wakeUpEvent),
        mpptMonitor(this, Wire), weatherSensors(this), gpio(this), command(this) {
}
#endif

bool System::begin(RadioEvents_t *radioEvents) {
    Log.infoln(F("[SYSTEM] Starting"));
    printJsonSystem(PSTR("Starting"));

    Wire.begin();

    pixels->begin();
    pixels->clear();

#ifdef USE_SCREEN
    if (!display->init()) {
        serialError(PSTR("[SYSTEM] Display error"));
    }
#endif

    turnScreenOn();

    turnOnRGB(COLOR_CYAN);

    EEPROM.begin(512);

    if (EEPROM.read(EEPROM_ADDRESS_VERSION) != EEPROM_VERSION) {
        setFunctionAllowed(EEPROM_ADDRESS_WATCHDOG_SAFETY, functionsAllowed[EEPROM_ADDRESS_WATCHDOG_SAFETY]);
        setFunctionAllowed(EEPROM_ADDRESS_APRS_DIGIPEATER, functionsAllowed[EEPROM_ADDRESS_APRS_DIGIPEATER]);
        setFunctionAllowed(EEPROM_ADDRESS_APRS_TELEMETRY, functionsAllowed[EEPROM_ADDRESS_APRS_TELEMETRY]);
        setFunctionAllowed(EEPROM_ADDRESS_APRS_POSITION, functionsAllowed[EEPROM_ADDRESS_APRS_POSITION]);
        setFunctionAllowed(EEPROM_ADDRESS_SLEEP, functionsAllowed[EEPROM_ADDRESS_SLEEP]);

        EEPROM.write(EEPROM_ADDRESS_VERSION, EEPROM_VERSION);
        EEPROM.commit();
    } else {
        functionsAllowed[EEPROM_ADDRESS_WATCHDOG_SAFETY] = EEPROM.read(EEPROM_ADDRESS_WATCHDOG_SAFETY);
        functionsAllowed[EEPROM_ADDRESS_APRS_DIGIPEATER] = EEPROM.read(EEPROM_ADDRESS_APRS_DIGIPEATER);
        functionsAllowed[EEPROM_ADDRESS_APRS_TELEMETRY] = EEPROM.read(EEPROM_ADDRESS_APRS_TELEMETRY);
        functionsAllowed[EEPROM_ADDRESS_APRS_POSITION] = EEPROM.read(EEPROM_ADDRESS_APRS_POSITION);
        functionsAllowed[EEPROM_ADDRESS_SLEEP] = EEPROM.read(EEPROM_ADDRESS_SLEEP);
    }

    sprintf_P(bufferText, PSTR("Watchdog safety: %d Aprs Digi : %d Telem : %d Position : %d Sleep : %d"),
            functionsAllowed[EEPROM_ADDRESS_WATCHDOG_SAFETY],
            functionsAllowed[EEPROM_ADDRESS_APRS_DIGIPEATER],
            functionsAllowed[EEPROM_ADDRESS_APRS_TELEMETRY],
            functionsAllowed[EEPROM_ADDRESS_APRS_POSITION],
            functionsAllowed[EEPROM_ADDRESS_SLEEP]
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

    if (SerialPi->available()) {
        streamReceived = SerialPi;
        Log.verboseln(F("Serial Pi incoming"));
    } else if (Serial.available()) {
        streamReceived = &Serial;
        Log.verboseln(F("Serial USB incoming"));
    }

    if (streamReceived != nullptr) {
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

            if (isFunctionAllowed(EEPROM_ADDRESS_SLEEP) && mpptMonitor.getVoltageBattery() < VOLTAGE_TO_SLEEP) {
                Log.infoln(F("[SLEEP] Battery is %dV and under of %dV so sleep"), mpptMonitor.getVoltageBattery(), VOLTAGE_TO_SLEEP);
                displayText(PSTR("Sleep"), PSTR("Battery too low so sleep"), 1000);
                sleep(SLEEP_DURATION);
            }
        }

        if (timerTelemetry.hasExpired() || timerPosition.hasExpired() || timerStatus.hasExpired()
            || forceSendTelemetry || forceSendPosition
        ) {
            communication->update(forceSendTelemetry || timerTelemetry.hasExpired(),
                                  forceSendPosition || timerPosition.hasExpired(),
                                  forceSendPosition || timerStatus.hasExpired());
            forceSendTelemetry = false;
            forceSendPosition = false;

            if (timerTelemetry.hasExpired()) {
                timerTelemetry.restart();
            }

            if (timerPosition.hasExpired()) {
                timerPosition.restart();
            }

            if (timerStatus.hasExpired()) {
                timerStatus.restart();
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

        Log.traceln(F("[LED] Turn led color : 0x%x"), color);

        uint8_t red, green, blue;
        red = (uint8_t) (color >> 16);
        green = (uint8_t) (color >> 8);
        blue = (uint8_t) color;
        pixels->setPixelColor(0, CubeCell_NeoPixel::Color(red, green, blue));
        pixels->show();

        delay(250);
    }
}

void System::turnOffRGB() {
    turnOnRGB(0);
}

void System::turnScreenOn() {
    if (!screenOn) {
        screenOn = true;
        Log.traceln(F("[SCREEN/LED] Turn screen/led on"));

#ifdef USE_SCREEN
        display->wakeup();
#endif

        timerScreen.restart();
    }
}

void System::turnScreenOff() {
    if (screenOn) {
        Log.traceln(F("[SCREEN/LED] Turn screen/led off"));
#ifdef USE_SCREEN
        display->sleep();
#endif

        screenOn = false;
    }

    turnOffRGB();
}

void System::displayText(const char *title, const char *content, uint16_t pause) const {
#ifdef USE_SCREEN
    if (!screenOn) {
        return;
    }

    Log.traceln(F("[Display] %s --> %s"), title, content);
    display->clear();
    display->drawString(0, 0, title);
    display->drawStringMaxWidth(0, 10, 120, content);
    display->display();
    delay(pause);
#endif
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
            .endObject(); SerialPi->println();

    Log.infoln(F("[TIME] %s. Temperature RTC: %dC"), bufferText, RTC.getTemperature());
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

void System::serialError(const char *content) {
    turnOnRGB(COLOR_VIOLET);
    Log.errorln(content);
    printJsonSystem((char*)content);
    delay(2000);
    turnOffRGB();
}

void System::setFunctionAllowed(byte function, bool allowed) {
    sprintf_P(bufferText, PSTR("[EEPROM] Set %d to %d"), function, allowed);
    Log.infoln(bufferText);
    displayText("EEPROM", bufferText);

    functionsAllowed[function] = allowed;

    EEPROM.write(function, allowed);
    EEPROM.commit();
}

void System::printJsonSystem(const char *state) {
    serialJsonWriter
            .beginObject()
            .property(F("type"), PSTR("system"))
            .property(F("state"), (char*)state)
            .property(F("boxOpened"), isBoxOpened())
            .property(F("temperatureRtc"), RTC.getTemperature())
            .property(F("watchdogSafety"), functionsAllowed[EEPROM_ADDRESS_WATCHDOG_SAFETY])
            .property(F("aprsDigipeater"), functionsAllowed[EEPROM_ADDRESS_APRS_DIGIPEATER])
            .property(F("aprsTelemetry"), functionsAllowed[EEPROM_ADDRESS_APRS_TELEMETRY])
            .property(F("aprsPosition"), functionsAllowed[EEPROM_ADDRESS_APRS_POSITION])
            .property(F("sleep"), functionsAllowed[EEPROM_ADDRESS_SLEEP])
            .endObject(); SerialPi->println();
}

void System::sleep(uint64_t time) {
    turnOnRGB(COLOR_BLUE);

    sprintf_P(bufferText, PSTR("[SLEEP] Sleep during %dms"), time);
    Log.infoln(bufferText);
    displayText("Sleep", bufferText);
    
    gpio.setNpr(false);
    gpio.setWifi(false);

    turnScreenOff();
    digitalWrite(Vext, HIGH); // 0V
    Radio.Sleep();
    TimerSetValue(wakeUpEvent, time);
    TimerStart(wakeUpEvent);

    lowPowerHandler();
}

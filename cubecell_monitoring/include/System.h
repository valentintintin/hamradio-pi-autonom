#ifndef CUBECELL_MONITORING_SYSTEM_H
#define CUBECELL_MONITORING_SYSTEM_H

#include <cstdint>
#include <CubeCell_NeoPixel.h>
#include <HT_SH1107Wire.h>
#include <DS3231.h>

#include "Communication.h"
#include "Timer.h"
#include "Config.h"
#include "Command.h"
#include "MpptMonitor.h"
#include "WeatherSensors.h"
#include "Gpio.h"

class System {
public:
    explicit System(SH1107Wire *display, CubeCell_NeoPixel *pixels);

    bool begin(RadioEvents_t *radioEvents);
    void update();
    void userButton();
    void setTimeFromRTcToInternalRtc(uint64_t epoch);
    bool isBoxOpened();

    void turnOnRGB(uint32_t color);
    void turnOffRGB();
    void displayText(const char* title, const char* content, uint16_t pause = DELAY_SCREEN_DISPLAYED) const;

    static void nowToString(char *result);

    Communication *communication;
    Command command;
    MpptMonitor mpptMonitor;
    WeatherSensors weatherSensors;
    Gpio gpio;
    DS3231 RTC;

    bool forceSendTelemetry = false;
private:
    char bufferText[256]{};
    bool screenOn = false;

    Timer timerPosition = Timer(INTERVAL_POSITION, true);
    Timer timerTelemetry = Timer(INTERVAL_REFRESH_APRS, false);
    Timer timerTime = Timer(INTERVAL_TIME, true);
    Timer timerScreen = Timer(TIME_SCREEN_ON);
    Timer timerBoxOpened = Timer(INTERVAL_ALARM_BOX_OPENED, true);
    Timer timerSecond = Timer(1000, true);

    CubeCell_NeoPixel *pixels;
    SH1107Wire *display;

    void timeUpdate();
    void turnScreenOn();
    void turnScreenOff();
};

#endif //CUBECELL_MONITORING_SYSTEM_H

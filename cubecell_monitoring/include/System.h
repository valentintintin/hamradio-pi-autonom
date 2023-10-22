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
    void setTimeFromRTcToInternalRtc(uint64_t epoch);
    bool isBoxOpened() const;
    void setFunctionAllowed(byte function, bool allowed);

    inline bool isFunctionAllowed(byte function) const {
        return functionsAllowed[function];
    }

    void turnOnRGB(uint32_t color);
    void turnOffRGB();
    void displayText(const char* title, const char* content, uint16_t pause = TIME_PAUSE_SCREEN) const;
    void serialError(const char* content) const;

    static DateTime nowToString(char *result);

    Communication *communication{};
    Command command;
    MpptMonitor mpptMonitor;
    WeatherSensors weatherSensors;
    Gpio gpio;
    DS3231 RTC;

    bool forceSendPosition = false;
    bool forceSendTelemetry = false;
private:
    bool screenOn = false;
    uint32_t ledColor = 0;
    bool functionsAllowed[4] = {true, true, true, true};

    Timer timerPosition = Timer(INTERVAL_POSITION_APRS, true);
    Timer timerTelemetry = Timer(INTERVAL_TELEMETRY_APRS, false);
    Timer timerTime = Timer(INTERVAL_TIME, true);
    Timer timerScreen = Timer(TIME_SCREEN_ON);
    Timer timerBoxOpened = Timer(INTERVAL_ALARM_BOX_OPENED_APRS, true);
    Timer timerSecond = Timer(1000, true);

    CubeCell_NeoPixel *pixels;
    SH1107Wire *display;

    void showTime();
    void turnScreenOn();
    void turnScreenOff();

    void printJsonSystem(const char *state) const;
};

#endif //CUBECELL_MONITORING_SYSTEM_H

#include <Arduino.h>
#include <hardware/rtc.h>

#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <timers.h>

#include <LittleFS.h>
#include <CommandParser.h>
#include <ArduinoLog.h>
#include <Wire.h>
#include <DS3231.h>

#include "hal/PicoGpioHal.hpp"
#include "controllers/RelayController.hpp"

#include "settings.h"
#include "utils/rp2040.h"
#include "utils/utils.h"
#include "config.h"

//                  COMMANDS, COMMAND_ARGS, COMMAND_NAME_LENGTH, COMMAND_ARG_SIZE, COMMAND_HLP_LENGTH, RESPONSE_SIZE
typedef CommandParser<16,       2,              16,                 200 ,             0,               128> MyCommandParser; // Response size > 200 bug
bool processCommand(Stream* stream, const char *command);

char bufferText[BUFFER_LENGTH];
MyCommandParser parser;
Settings settings;

QueueHandle_t queueRelay = xQueueCreate(2, sizeof(RelayCommand));
RelayController relayController(&queueRelay);

bool isDST(const int16_t year, const int8_t month, const int8_t day, const int8_t hour) {
    if (month < 3 || month > 10) return false;
    if (month > 3 && month < 10) return true;

    const int lastSunday = 31 - ((5 * year / 4 + 4) % 7); // formule pour dernier dimanche

    if (month == 3) {
        // Passage à l'heure d'été
        if (day < lastSunday) return false;
        if (day > lastSunday) return true;
        return hour >= 2;
    } else if (month == 10) {
        // Retour à l'heure d'hiver
        if (day < lastSunday) return true;
        if (day > lastSunday) return false;
        return hour < 3;
    }

    return false;
}

int daysInMonth(const int8_t month, const int16_t year) {
    if (month == 2) return (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)) ? 29 : 28;
    if (month == 4 || month == 6 || month == 9 || month == 11) return 30;
    return 31;
}

void addHours(int16_t &year, int8_t &month, int8_t &day, int8_t &hour, const int8_t hoursToAdd) {
    hour += hoursToAdd;
    while (hour >= 24) {
        hour -= 24;
        day += 1;
        int dim = daysInMonth(month, year);
        if (day > dim) {
            day = 1;
            month += 1;
            if (month > 12) {
                month = 1;
                year += 1;
            }
        }
    }
}

DateTime getDateTime() {
    datetime_t datetime;
    rtc_get_datetime(&datetime);

    int16_t year = datetime.year + 1900;
    int8_t month = datetime.month;
    int8_t day = datetime.day;
    int8_t hour = datetime.hour;

    const auto isDSTNow = isDST(year, month, day, hour);
    addHours(year, month, day, hour, isDSTNow ? 2 : 1);

    return {static_cast<uint16_t>(year), static_cast<uint8_t>(month), static_cast<uint8_t>(day), static_cast<uint8_t>(hour), static_cast<uint8_t>(datetime.min), static_cast<uint8_t>(datetime.sec)};
}

void rebootTask(TimerHandle_t xTimer) {
    rp2040.reboot();
}

void dfuTask(TimerHandle_t xTimer) {
    rp2040.rebootToBootloader();
}

void doRebootOutput(MyCommandParser::Argument *args, char *response) {
    if (const TimerHandle_t timer = xTimerCreate("timerReboot", pdMS_TO_TICKS(10000), pdTRUE, nullptr, rebootTask); xTimerStart(timer, 0) == pdPASS) {
        strncpy(response, "Reboot in 10s !", MyCommandParser::MAX_RESPONSE_SIZE);
    } else {
        strncpy(response, "Reboot !", MyCommandParser::MAX_RESPONSE_SIZE);
        rebootTask(timer);
    }
}

void doDfuOutput(MyCommandParser::Argument *args, char *response) {
    if (const TimerHandle_t timer = xTimerCreate("timerDfu", pdMS_TO_TICKS(10000), pdTRUE, nullptr, dfuTask); xTimerStart(timer, 0) == pdPASS) {
        strncpy(response, "DFU in 10s !", MyCommandParser::MAX_RESPONSE_SIZE);
    } else {
        strncpy(response, "DFU !", MyCommandParser::MAX_RESPONSE_SIZE);
        dfuTask(timer);
    }
}

void doGpioOutput(MyCommandParser::Argument *args, char *response) {
    const RelayCommand command = {
        .id = static_cast<uint8_t>(args[0].asUInt64),
        .state = args[1].asUInt64 == 1,
    };
    xQueueSend(queueRelay, &command, 0);
    snprintf(response, MyCommandParser::MAX_RESPONSE_SIZE, "OK. GPIO %d is %d", command.id, command.state);
}

void doResetReasonOutput(MyCommandParser::Argument *args, char *response) {
    snprintf(response, MyCommandParser::MAX_RESPONSE_SIZE, "Reset reason: %d", rp2040.getResetReason());
}

void doUptimeOutput(MyCommandParser::Argument *args, char *response) {
    snprintf(response, MyCommandParser::MAX_RESPONSE_SIZE, "%lu seconds", millis() / 1000);
}

void doPingOutput(MyCommandParser::Argument *args, char *response) {
    char dateString[64];
    getDateTimeStringFromEpoch(getDateTime().unixtime(), dateString, 64);
    snprintf(response, MyCommandParser::MAX_RESPONSE_SIZE, "Pong!\n%s", dateString);
}

void heartBeatTask(void *pvParameters) {
    for (;;) {
        digitalWrite(LED_BUILTIN, HIGH);
        vTaskDelay(pdMS_TO_TICKS(500));
        digitalWrite(LED_BUILTIN, LOW);
        vTaskDelay(pdMS_TO_TICKS(500));

        rp2040.wdt_reset();
    }
}

void serialReceivedTask(void *pvParameters) {
    char response[MyCommandParser::MAX_RESPONSE_SIZE]{};
    parser.registerCommand("gpio", "uu", &doGpioOutput);
    parser.registerCommand("reboot", "", &doRebootOutput);
    parser.registerCommand("dfu", "", &doDfuOutput);
    parser.registerCommand("uptime", "", &doUptimeOutput);
    parser.registerCommand("resetReason", "", &doResetReasonOutput);
    parser.registerCommand("ping", "", &doPingOutput);

    Log.info(">");

    while (true) {
        Stream *streamReceived = nullptr;

        if (Serial.available()) {
            streamReceived = &Serial;
            Log.traceln("Serial USB incoming");
        } else if (Serial1.available()) {
            streamReceived = &Serial1;
            Log.traceln("Serial UART 0 incoming");
        }

        if (streamReceived != nullptr) {
            streamReceived->readBytesUntil('\n', bufferText, BUFFER_LENGTH);

            Log.infoln("Serial received: %s", bufferText);

            if (parser.processCommand(bufferText, response)) {
                Log.infoln("Command parsing OK: %s", response);
            } else {
                Log.warningln("Command parsing KO: %s", response);
            }

            memset(bufferText, 0, BUFFER_LENGTH);
            Log.info(">");
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void loadFromSettings() {
    uint8_t i = 0;
    settings.pins[i].pin = 11;
    strcpy(settings.pins[i++].name, "wifi");
    settings.pins[i].pin = 12;
    strcpy(settings.pins[i++].name, "linux");
    settings.pins[i].pin = 10;
    strcpy(settings.pins[i++].name, "msh");

    for (const auto pin : settings.pins) {
        if (pin.i2cAddress == 0) {
            relayController.addRelay(new PicoGpioHal(pin.pin, pin.mode, pin.inverted));
        }
    }
}

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
    randomSeed(analogRead(A1));

    if (settings.useSlowClock) {
        setSlowClock();
    }

    Serial.begin(115200);
    Serial1.begin(115200);

    Log.begin(LOG_LEVEL_TRACE, &Serial);
    Log.addHandler(&Serial1);

    if (rp2040.getResetReason() == RP2040::WDT_RESET) {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(2500);
        Log.warningln("Watchdog caused reboot: %d"), rp2040.getResetReason();
        digitalWrite(LED_BUILTIN, LOW);
    }

    Log.infoln("Starting");

    if (settings.useInternalWatchdog) {
        rp2040.wdt_begin(8300);
        Log.infoln("[SYSTEM] Internal watchdog enabled");
    }

    LittleFS.begin();
    rtc_init();
    Wire.begin();

    if (settings.rtc.enabled) {
        if (const auto now = RTClib::now(); now.year() >= 2025 && now.year() <= 2060) {
            const auto epoch = now.unixtime();
            setTimeToInternalRtc(epoch);
            getDateTimeStringFromEpoch(epoch, bufferText, BUFFER_LENGTH);
            Log.infoln("Set internal RTC to date %s", bufferText);
        } else {
            setTimeToInternalRtc(0);
            Log.warningln("Wrong rtc time !");
        }
    } else {
        setTimeToInternalRtc(0);
    }

    xTaskCreate(heartBeatTask, "HeartBeat", configMINIMAL_STACK_SIZE, nullptr, tskIDLE_PRIORITY, nullptr);

    loadFromSettings();

    relayController.begin();

    xTaskCreate(serialReceivedTask, "SerialReceived", configMINIMAL_STACK_SIZE, nullptr, tskIDLE_PRIORITY, nullptr);
}

void loop() {
}
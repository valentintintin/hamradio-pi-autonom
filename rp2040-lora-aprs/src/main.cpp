#include <Arduino.h>
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <hardware/rtc.h>
#include <LittleFS.h>
#include <CommandParser.h>
#include <ArduinoLog.h>
#include <Wire.h>
#include <DS3231.h>

#include "PicoGpioHal.hpp"
#include "RelayController.hpp"

#include "settings.h"
#include "rp2040.h"
#include "utils.h"
#include "config.h"

//                  COMMANDS, COMMAND_ARGS, COMMAND_NAME_LENGTH, COMMAND_ARG_SIZE, COMMAND_HLP_LENGTH, RESPONSE_SIZE
typedef CommandParser<32,       2,              16,                     200,                0,              200> MyCommandParser;

bool processCommand(Stream* stream, const char *command);

char bufferText[BUFFER_LENGTH];
MyCommandParser parser;
Settings settings;

QueueHandle_t queueRelay = xQueueCreate(2, sizeof(RelayCommand));
RelayController relayController(&queueRelay);

void doGpioOutput(MyCommandParser::Argument *args, char *response) {
    const RelayCommand command = {
        .id = static_cast<uint8_t>(args[0].asUInt64),
        .state = args[1].asUInt64 == 1,
    };
    xQueueSend(queueRelay, &command, 0);
    snprintf_P(response, MyCommandParser::MAX_RESPONSE_SIZE, PSTR("OK. GPIO %d is %d"), command.id, command.state);
}

void heartBeatTask(void *pvParameters) {
    pinMode(LED_BUILTIN, OUTPUT);
    for (;;) {
        digitalWrite(LED_BUILTIN, HIGH);
        vTaskDelay(pdMS_TO_TICKS(500));
        digitalWrite(LED_BUILTIN, LOW);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void serialReceivedTask(void *pvParameters) {
    char response[MyCommandParser::MAX_RESPONSE_SIZE + 1]{};
    parser.registerCommand(PSTR("gpio"), PSTR("uu"), doGpioOutput);

    Log.info(F(">"));

    while (true) {
        Stream *streamReceived = nullptr;

        if (Serial.available()) {
            streamReceived = &Serial;
            Log.traceln(F("Serial USB incoming"));
        } else if (Serial1.available()) {
            streamReceived = &Serial1;
            Log.traceln(F("Serial UART 0 incoming"));
        }

        if (streamReceived != nullptr) {
            const auto serialReceived = streamReceived->readString().c_str();

            Log.infoln(F("Serial received: %s"), serialReceived);

            if (parser.processCommand(serialReceived, response)) {
                Log.warningln(F("Command parsing failed"));
                Serial.println(F("KO"));
            } else {
                Log.infoln(F("Command parsing OK: %s"), response);
            }


            Log.info(F(">"));
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void loadFromSettings() {
    uint8_t i = 0;
    settings.pins[i].pin = 11;
    // strcpy_P(settings.pins[i++].name, PSTR("wifi"));
    settings.pins[i].pin = 12;
    // strcpy_P(settings.pins[i++].name, PSTR("linux"));
    settings.pins[i].pin = 10;
    // strcpy_P(settings.pins[i++].name, PSTR("msh"));

    for (const auto pin : settings.pins) {
        if (pin.i2cAddress == 0) {
            relayController.addRelay(new PicoGpioHal(pin.pin, pin.mode, pin.inverted));
        }
    }
}

void setup() {
    // randomSeed(analogRead(A1));
    //
    // if (settings.useSlowClock) {
    //     setSlowClock();
    // }
    //
    // Serial.begin(115200);
    // Serial1.begin(115200);
    //
    // Log.begin(LOG_LEVEL_TRACE, &Serial);
    // Log.addHandler(&Serial1);
    //
    // if (watchdog_enable_caused_reboot()) {
    //     Log.warningln(F("Watchdog caused reboot"));
    // }
    //
    // Log.infoln(F("Starting"));

    // if (settings.useInternalWatchdog) {
        // rp2040.wdt_begin(8300);
        // Log.infoln(F("[SYSTEM] Internal watchdog enabled"));
    // }

    // LittleFS.begin();
    // rtc_init();
    // Wire.begin();

    // if (settings.rtc.enabled) {
    //     if (const auto now = RTClib::now(); now.year() >= 2025 && now.year() <= 2060) {
    //         const auto epoch = now.unixtime();
    //         setTimeToInternalRtc(epoch);
    //         getDateTimeStringFromEpoch(epoch, bufferText, BUFFER_LENGTH);
    //         Log.infoln(F("Set internal RTC to date %s"), bufferText);
    //     } else {
    //         setTimeToInternalRtc(0);
    //         Log.warningln(F("Wrong rtc time !"));
    //     }
    // } else {
    //     setTimeToInternalRtc(0);
    // }

    xTaskCreate(heartBeatTask, "HeartBeat", 512, nullptr, 1, nullptr);

    // loadFromSettings();

    // relayController.begin();

    // xTaskCreate(serialReceivedTask, "SerialReceived", 512, nullptr, 1, nullptr);

    vTaskStartScheduler();
}

void loop() {
}

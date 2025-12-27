#include <Arduino.h>
#include <hardware/rtc.h>

#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>

#include <LittleFS.h>
#include <ArduinoLog.h>
#include <Wire.h>
#include <DS3231.h>

#include "hal/PicoGpioHal.hpp"
#include "controllers/RelayController.hpp"

#include "settings.h"
#include "utils/rp2040.h"
#include "utils/utils.h"
#include "config.h"
#include "SettingsManager.hpp"
#include "controllers/CommandController.hpp"
#include "controllers/I2CSlaveController.hpp"
#include "controllers/SensorController.hpp"
#include "controllers/WatchdogController.hpp"

char bufferText[BUFFER_LENGTH];

void serialReceivedTask(void* pvParameters)
{
    Log.info(">");

    while (true)
    {
        Stream* streamReceived = nullptr;

        if (Serial.available())
        {
            streamReceived = &Serial;
            Log.traceln("Serial USB incoming");
        }
        else if (Serial1.available())
        {
            streamReceived = &Serial1;
            Log.traceln("Serial UART 0 incoming");
        }

        if (streamReceived != nullptr)
        {
            streamReceived->readBytesUntil('\r', bufferText, BUFFER_LENGTH);
            while (streamReceived->available())
            {
                streamReceived->read();
            }

            Log.infoln("Serial received: %s", bufferText);

            if (CommandController::getInstance().processCommand(bufferText))
            {
                Log.infoln("Command parsing OK: %s", CommandController::getInstance().getResponse());
            }
            else
            {
                Log.warningln("Command parsing KO: %s", CommandController::getInstance().getResponse());
            }

            memset(bufferText, 0, BUFFER_LENGTH);
            Log.info(">");
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void setup()
{
    pinMode(LED_BUILTIN, OUTPUT);
    randomSeed(analogRead(A1));

    Serial.begin(115200);
    Serial.setTimeout(5000);
    Serial1.begin(115200);
    Serial1.setTimeout(5000);

    Log.begin(LOG_LEVEL_VERBOSE, &Serial);
    Log.addHandler(&Serial1);

    if (rp2040.getResetReason() == RP2040::WDT_RESET)
    {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(2500);
        Log.warningln("Watchdog caused reboot: %d"), rp2040.getResetReason();
        digitalWrite(LED_BUILTIN, LOW);
    }

    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
    digitalWrite(LED_BUILTIN, LOW);
    delay(500);

    Log.infoln("Starting");

    rtc_init();

    Wire.setSDA(0);
    Wire.setSCL(1);

    Wire1.setSDA(2);
    Wire1.setSCL(3);

    SettingsManager::getInstance().begin();

    if (SettingsManager::getSettings().useSlowClock)
    {
        setSlowClock();
    }

    if (const auto now = RTClib::now(); now.year() >= 2025 && now.year() <= 2060)
    {
        const auto epoch = now.unixtime();
        setTimeToInternalRtc(epoch);
        getDateTimeStringFromEpoch(epoch, bufferText, BUFFER_LENGTH);
        Log.infoln("Set internal RTC to date %s", bufferText);
    }
    else
    {
        setTimeToInternalRtc(0);
        Log.warningln("Wrong rtc time !");
    }

    RelayController::getInstance().begin();
    SensorController::getInstance().begin();
    WatchdogController::getInstance().begin();
    I2CSlaveController::getInstance().begin();
    CommandController::getInstance().begin();

    if (xTaskCreate(serialReceivedTask, "SerialReceived", configMINIMAL_STACK_SIZE, nullptr, tskIDLE_PRIORITY, nullptr) != pdPASS)
    {
        Log.errorln("Serial task KO");
    }

    // pinMode(LED_BUILTIN,  OUTPUT);
    // pinMode(0,  OUTPUT);
    // pinMode(1,  OUTPUT);
    // pinMode(4,  OUTPUT);
    // pinMode(5,  OUTPUT);

    SensorController::getInstance().queryTelemetries();
}

void loop()
{
    // digitalWrite(LED_BUILTIN, HIGH);
    // delay(500);
    // digitalWrite(LED_BUILTIN, LOW);
    // delay(500);
    //
    // Serial.println(">");
    //
    // while (Serial.available()) {
    //     Serial.write(Serial.read());
    // }
    //
    // delay(100);

    // SensorController::getInstance().begin();

    // digitalWrite(LED_BUILTIN, true);
    // digitalWrite(0, true);
    // digitalWrite(1, true);
    // digitalWrite(4, true);
    // digitalWrite(5, true);
    // delay(1000);
    //
    // digitalWrite(LED_BUILTIN, false);
    // digitalWrite(0, false);
    // digitalWrite(1, false);
    // digitalWrite(4, false);
    // digitalWrite(5, false);
    // delay(5000);
}

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
            streamReceived->readBytesUntil('\n', bufferText, BUFFER_LENGTH);

            Log.infoln("Serial received: %s", bufferText);

            // if (commandController.processCommand(bufferText))
            // {
                // Log.infoln("Command parsing OK: %s", commandController.getResponse());
            // }
            // else
            // {
                // Log.warningln("Command parsing KO: %s", commandController.getResponse());
            // }

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
    Serial1.begin(115200);

    Log.begin(LOG_LEVEL_TRACE, &Serial);
    Log.addHandler(&Serial1);

    if (rp2040.getResetReason() == RP2040::WDT_RESET)
    {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(2500);
        Log.warningln("Watchdog caused reboot: %d"), rp2040.getResetReason();
        digitalWrite(LED_BUILTIN, LOW);
    }

    Log.infoln("Starting");

    rtc_init();
    Wire.begin();

    SettingsManager::getInstance().begin();

    if (SettingsManager::getSettings().useSlowClock)
    {

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

    WatchdogController::getInstance().begin();
    RelayController::getInstance().begin();
    SensorController::getInstance().begin();
    I2CSlaveController::getInstance().begin();
    CommandController::getInstance().begin();

    if (xTaskCreate(serialReceivedTask, "SerialReceived", configMINIMAL_STACK_SIZE, nullptr, tskIDLE_PRIORITY, nullptr) != pdPASS)
    {
        Log.errorln("Serial task cr");
    }
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
}

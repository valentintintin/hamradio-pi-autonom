#include <Arduino.h>
#include <hardware/rtc.h>

#include <FreeRTOS.h>
#include <task.h>

#include <LittleFS.h>
#include <ArduinoLog.h>
#include <Wire.h>
#include <DS3231.h>

#include "controllers/RelayController.hpp"

#include "settings.h"
#include "utils/rp2040.h"
#include "utils/utils.h"
#include "config.h"
#include "SettingsManager.hpp"
#include "controllers/CommandController.hpp"
#include "controllers/I2CSlaveController.hpp"
#include "controllers/LedController.hpp"
#include "controllers/SensorController.hpp"
#include "controllers/WatchdogController.hpp"
#include "hal/I2CMasterHal.hpp"
#include "hal/LoRaHal.hpp"

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

void setMpptVoltageLimits()
{
    const auto& settingsMppt = SettingsManager::getSettings().mppt;

    if (!I2CMasterHal::takeSemaphore())
    {
        Log.warningln("Can not set relay Mppt Charger voltage limit, can not have semaphore");

        LedController::getInstance().blink(LedError, LedI2C);

        return;
    }

    if (!MpptChargerHal::getInstance().setVoltageLimits(settingsMppt.powerOffVoltage, settingsMppt.powerOnVoltage))
    {
        Log.warningln("Can not set relay Mppt Charger voltage limit");

        LedController::getInstance().blink(LedError, LedMpptCharger);
    }

    I2CMasterHal::releaseSemaphore();
}

void setLora()
{
    const auto settingsLora = SettingsManager::getSettings().lora;

    const SettingsLoRaModem lora = settingsLora.modems[settingsLora.mode];

    if (lora.enabled)
    {
        LoRaHal::getInstance().begin(
            settingsLora.txEnabled,
            lora.frequency,
            lora.bandwidth,
            lora.spreadingFactor,
            lora.codingRate,
            lora.syncWord,
            settingsLora.outputPower,
            lora.preambleLength
        );
    }
}

void setup()
{
    pinMode(LED_BUILTIN, OUTPUT);
    randomSeed(analogRead(A1));

    Serial.begin(115200);
    Serial.setTimeout(5000);

    // Serial1.begin(115200);
    // Serial1.setTimeout(5000);

    Serial2.begin(115200);
    Serial2.setTimeout(5000);

    Log.begin(LOG_LEVEL_VERBOSE, &Serial);
    Log.addHandler(&Serial2);

    digitalWrite(LED_BUILTIN, HIGH);
    delay(2500);
    digitalWrite(LED_BUILTIN, LOW);
    delay(2500);

    const auto resetReason = rp2040.getResetReason();
    Log.infoln("Reboot reason: %d", resetReason);

    if (resetReason == RP2040::WDT_RESET)
    {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(1000);
        digitalWrite(LED_BUILTIN, LOW);
        delay(1000);
    }

    Log.infoln("Starting");

    LedController::getInstance().begin();

    rtc_init();

    Wire.setSDA(0);
    Wire.setSCL(1);
    Wire.begin();

    Wire1.setSDA(2);
    Wire1.setSCL(3);

    // Initialisation SPI pour LoRa (SPI1)
    SPI1.setRX(LORA_MISO);
    SPI1.setTX(LORA_MOSI);
    SPI1.setSCK(LORA_SCK);

    SettingsManager::getInstance().begin();

    if (SettingsManager::getSettings().useSlowClock)
    {
        setSlowClock();
    }

    const auto now = RTClib::now();

    if (now.year() >= 2026 && now.year() <= 2060)
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

        LedController::getInstance().blink(LedError, LedClock);
    }

    RelayController::getInstance().begin();
    SensorController::getInstance().begin();
    WatchdogController::getInstance().begin();
    I2CSlaveController::getInstance().begin();
    CommandController::getInstance().begin();

    setMpptVoltageLimits();
    SensorController::getInstance().queryTelemetries();

    if (xTaskCreate(serialReceivedTask, "SerialReceived", configMINIMAL_STACK_SIZE, nullptr, tskIDLE_PRIORITY, nullptr) != pdPASS)
    {
        Log.errorln("Serial task KO");

        LedController::getInstance().blink(LedError, LedFreeRtos);
    }

    setLora();
}

void loop()
{
}

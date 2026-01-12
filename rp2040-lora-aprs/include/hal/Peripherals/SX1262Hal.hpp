#pragma once

#include <Arduino.h>
#include <FreeRTOS.h>
#include <queue.h>

#include "modules/SX126x/SX1262.h"

#include "config.h"
#include "hal/Hal.hpp"

#define TRX_BUFFER 256
#define LORA_QUEUE_TX_SIZE 10
#define LORA_QUEUE_RX_SIZE 10
#define TIMEOUT_CAD_DETECTION 1000

struct LoRaTxMessage
{
    uint8_t data[TRX_BUFFER];
    size_t length;
};

struct LoRaRxMessage
{
    uint8_t data[TRX_BUFFER];
    size_t length;
    float rssi;
    float snr;
};

class SX1262Hal // : public Hal TODO héir
{
public:
    static SX1262Hal& getInstance()
    {
        static SX1262Hal instance;
        return instance;
    }

    bool begin(bool txEnabled,
               float frequency,
               uint16_t bandwidth,
               uint8_t spreadingFactor,
               uint8_t codingRate,
               uint8_t syncWord,
               uint8_t outputPower,
               uint8_t preambleLength = RADIOLIB_SX126X_CMD_SET_TX_INFINITE_PREAMBLE);

    bool send(const uint8_t* data, size_t length);
    bool receive(LoRaRxMessage& message, TickType_t timeout = portMAX_DELAY);

    bool startChannelScan();

    bool isInitialized() const
    {
        return initialized;
    }

private:
    static void txTask(void* pvParameters);
    static void rxTask(void* pvParameters);

    static void onRxInterrupt();
    static void onTxInterrupt();
    static void onCadInterrupt();

    QueueHandle_t txQueue;
    QueueHandle_t rxQueue;
    TaskHandle_t txTaskHandle = nullptr;
    TaskHandle_t rxTaskHandle = nullptr;

    bool initialized = false;
    bool txEnabled = true;
    SX1262 radio = SX1262(new Module(LORA_CS, LORA_DIO1, LORA_RESET, LORA_BUSY, SPI1));

    SX1262Hal();

    void processTxQueue();
    void processRxQueue();

    bool startReceive();
    bool standby();
};

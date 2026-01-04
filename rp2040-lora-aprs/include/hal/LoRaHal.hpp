#pragma once

#include <Arduino.h>
#include <FreeRTOS.h>
#include <queue.h>

#include "modules/SX126x/SX1262.h"

#include "config.h"

#define TRX_BUFFER 256
#define LORA_QUEUE_TX_SIZE 10
#define LORA_QUEUE_RX_SIZE 10

struct LoRaTxMessage
{
    uint8_t* data;
    size_t length;
};

struct LoRaRxMessage
{
    uint8_t data[TRX_BUFFER];
    size_t length;
    float rssi;
    float snr;
};

class LoRaHal
{
public:
    static LoRaHal& getInstance()
    {
        static LoRaHal instance;
        return instance;
    }

    bool begin(float frequency,
               uint16_t bandwidth,
               uint8_t spreadingFactor,
               uint8_t codingRate,
               uint8_t outputPower,
               uint8_t syncWord);

    bool send(const uint8_t* data, size_t length);
    bool receive(LoRaRxMessage& message, TickType_t timeout = portMAX_DELAY);

    bool startChannelScan();

    bool isInitialized() const
    {
        return initialized;
    }

private:
    LoRaHal();

    static void txTask(void* pvParameters);
    static void rxTask(void* pvParameters);

    void processTxQueue();
    void processRxQueue();

    QueueHandle_t txQueue;
    QueueHandle_t rxQueue;
    TaskHandle_t txTaskHandle = nullptr;
    TaskHandle_t rxTaskHandle = nullptr;

    bool initialized = false;
    bool txEnabled = true;
    bool cadResult = false;
    bool cadPending = false;
    SX1262 radio = SX1262(new Module(LORA_CS, LORA_DIO1, LORA_RESET, LORA_BUSY, SPI1,
                             SPISettings(4000000, MSBFIRST, SPI_MODE0)));

    static void onRxInterrupt();
    static void onTxInterrupt();
    static void onCadInterrupt();
};

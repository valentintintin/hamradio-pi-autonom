#ifndef RADIO_H
#define RADIO_H

#include <RadioLib.h>
#include <FreeRTOS.h>
#include <queue.h>
#include <semphr.h>

#include "../include/config.h"

#define IRQS (RADIOLIB_IRQ_CAD_DETECTED | RADIOLIB_IRQ_CAD_DONE | RADIOLIB_IRQ_CRC_ERR | RADIOLIB_IRQ_RX_DONE | RADIOLIB_IRQ_TX_DONE | RADIOLIB_IRQ_TIMEOUT)

class System;

typedef struct {
    uint8_t payload[TRX_BUFFER];
    uint16_t size;
    float rssi;
    float snr;
} LoRaReceived;

typedef struct {
    uint8_t payload[TRX_BUFFER];
    uint16_t size;
} LoRaTransmit;

enum InterruptType {
    Pending,
    CadDetected,
    CadDone,
    RxDone,
    TxDone,
    RxTimeout,
    CrcError
};

enum RadioState {
    IDLE,
    RX,
    TX
};

class Radio {
public:
    explicit Radio();

    bool init();

    bool send(const uint8_t *payload, size_t size);

    bool changeLoRaSettings(float frequency, uint16_t bandwidth, uint8_t spreadingFactor, uint8_t codingRate,
                            uint8_t outputPower, uint8_t syncWord = 0x12, bool boostedRxGain = true);

    bool hasError() const {
        return _hasError;
    }

    uint32_t countRx() const {
        return nbRx;
    }

    uint32_t countTx() const {
        return nbTx;
    }

private:
    static volatile RadioState radioStatus;
    static QueueHandle_t irqQueue;

    static void onISR();

    SX1262 lora = new Module(LORA_CS, LORA_DIO1, LORA_RESET, LORA_BUSY, SPI1,
                             SPISettings(4000000, MSBFIRST, SPI_MODE0));

    QueueHandle_t rxQueue;
    QueueHandle_t txQueue;
    SemaphoreHandle_t txDone;

    bool _hasError = false;
    bool loraSettingsChanged = false;

    uint32_t slotTimeMsec = 0;
    uint32_t maxPacketTimeMsec = 0;

    uint32_t nbTx = 0;
    uint32_t nbRx = 0;

    bool handleReceived();

    bool startSend(const uint8_t *payload, uint16_t size);

    bool startReceive();

    bool setStandby();

    InterruptType getAndClearIrq();

    void processInterruptTask();

    void receiveTask();

    void transmitTask();

    static void taskProcessInterruptRun(void *params);

    static void taskReceiveRun(void *params);

    static void taskTransmitRun(void *params);

    uint32_t getTxDelayMsec() const;

    uint32_t computeSlotTimeMsec(float bw, uint8_t sf);

    uint32_t getPacketTime(float bw, uint8_t sf, uint8_t cr, uint16_t preambleLength, uint32_t packetSize);
};

#endif //RADIO_H

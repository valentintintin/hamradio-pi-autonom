#ifndef RADIO_H
#define RADIO_H

#include <RadioLib.h>
#include <ArduinoQueue.h>
#include <MyThread.h>
#include <Timer.h>

#include "config.h"

class System;

typedef struct {
    uint8_t payload[TRX_BUFFER + 1];
    uint16_t size;
    float rssi;
    float snr;
} LoRaReceived;

typedef struct {
    uint8_t payload[TRX_BUFFER + 1];
    uint16_t size;
} LoRaTransmit;

enum InterruptType { IDLE, RX, TX };

class Radio : public MyThread {
public:
    explicit Radio(System *system);

    bool init() override;
    bool runOnce() override;
    bool send(const uint8_t *payload, size_t size);

    bool changeLoRaSettings(float frequency, uint16_t bandwidth, uint8_t spreadingFactor, uint8_t codingRate, uint8_t outputPower, bool boostedRxGain = true);

    bool hasError() const override {
      return _hasError || MyThread::hasError();
    }

    inline uint16_t countRxItemQueued() {
        return rxQueue.itemCount();
    }

    inline uint16_t countTxItemQueued() {
        return txQueue.itemCount();
    }

    inline uint32_t countRx() const {
        return nbRx;
    }

    inline uint32_t countTx() const {
        return nbTx;
    }
private:
    static volatile InterruptType hasInterrupt;

    static void setHasRxInterrupt();
    static void setHasTxInterrupt();

    SX1262 lora = new Module(LORA_CS, LORA_DIO1, LORA_RESET, LORA_BUSY, SPI1, SPISettings(4000000, MSBFIRST, SPI_MODE0));
    ArduinoQueue<LoRaReceived> rxQueue = ArduinoQueue<LoRaReceived>(LORA_QUEUE_RX_SIZE);
    ArduinoQueue<LoRaTransmit> txQueue = ArduinoQueue<LoRaTransmit>(LORA_QUEUE_TX_SIZE);
    bool _hasError = false;
    bool loraSettingsChanged = false;
    /// are _trying_ to receive a packet currently (note - we might just be waiting for one)
    bool wantToSend = false;
    InterruptType radioStatus = IDLE;
    uint32_t activeReceiveStart = 0;
    uint32_t lastTxStart = 0;
    Timer timerNextTx = Timer();
    uint32_t slotTimeMsec = 0;
    uint32_t preambleTimeMsec = 0;
    uint32_t maxPacketTimeMsec = 0;
    uint32_t nbTx = 0;
    uint32_t nbRx = 0;

    bool handleReceived();
    void completeSending();
    bool startSend(const uint8_t * payload, uint16_t size);
    bool isChannelActive();
    bool canSendImmediately();
    bool startReceive();
    bool setStandby();
    bool receiveDetected(uint16_t irq, ulong syncWordHeaderValidFlag, ulong preambleDetectedFlag);

    inline bool isReceiving() const {
        return radioStatus == RX;
    }

    inline bool isSending() const {
        return radioStatus == TX;
    }

    uint32_t getTxDelayMsec() const;
    uint32_t computeSlotTimeMsec(float bw, uint8_t sf);
    uint32_t getPacketTime(float bw, uint8_t sf, uint8_t cr, uint16_t preambleLength, uint32_t packetSize);
};

#endif //RADIO_H

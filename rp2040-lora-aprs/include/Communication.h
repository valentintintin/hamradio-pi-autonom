#ifndef RP2040_LORA_APRS_COMMUNICATION_H
#define RP2040_LORA_APRS_COMMUNICATION_H

#include "Aprs.h"
#include "config.h"

class System;

class Communication {
public:
    explicit Communication(System *system);

    void received(const uint8_t * payload, uint16_t size, float rssi, float snr);
    bool sendMessage(const char* destination, const char* message, const char* ackToConfirm = nullptr);
    bool sendPosition(const char* comment);
    bool sendStatus(const char* comment);
    bool sendTelemetry();
    bool sendTelemetryParams();
    bool sendItem(const char* name, char symbol, char symbolTable, const char* comment, double latitude, double longitude, uint16_t altitude, bool alive = true);

    bool shouldSendTelemetryParams = false;
private:
    System* system;

    AprsPacket aprsPacketTx{};
    AprsPacketLite aprsPacketRx{};

    bool sendAprsFrame();
    void prepareTelemetry();
};

#endif //RP2040_LORA_APRS_COMMUNICATION_H

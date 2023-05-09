#ifndef CUBECELL_MONITORING_COMMUNICATION_H
#define CUBECELL_MONITORING_COMMUNICATION_H

#include <radio/radio.h>
#include "Config.h"
#include "../lib/Aprs/Aprs.h"

class System;

class Communication {
public:
    explicit Communication(System *system);

    bool begin(RadioEvents_t *radioEvents);
    void update(bool sendTelemetry, bool sendPosition);
    void sent();
    void received(uint8_t * payload, uint16_t size, int16_t rssi, int8_t snr);

    void sendMessage(const char* destination, const char* message, const char* ackToConfirm = nullptr);
    void sendPosition(const char* comment);
    void sendTelemetry();
private:
    System* system;

    uint8_t buffer[TRX_BUFFER]{};
    char bufferText[TRX_BUFFER]{};
    AprsPacket aprsPacketTx, aprsPacketRx;
    uint16_t telemetrySequenceNumber = 0;
    uint8_t ackToAsk = 1;

    void send();
};

#endif //CUBECELL_MONITORING_COMMUNICATION_H

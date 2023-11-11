#ifndef CUBECELL_MONITORING_COMMUNICATION_H
#define CUBECELL_MONITORING_COMMUNICATION_H

#include <radio/radio.h>
#include "../lib/Aprs/Aprs.h"

#include "Config.h"

class System;

class Communication {
public:
    explicit Communication(System *system);

    bool begin(RadioEvents_t *radioEvents);
    void update(bool sendTelemetry, bool sendPosition, bool sendStatus);
    void sent();
    void received(uint8_t * payload, uint16_t size, int16_t rssi, int8_t snr);

    void sendMessage(const char* destination, const char* message, const char* ackToConfirm = nullptr);
    void sendPosition(const char* comment);
    void sendStatus(const char* comment);
    void sendTelemetry();
    void sendTelemetryParams();

    bool shouldSendTelemetryParams = false;
private:
    System* system;

    uint8_t buffer[TRX_BUFFER + 3]{};
    AprsPacket aprsPacketTx;
    AprsPacketLite aprsPacketRx;
    uint16_t telemetrySequenceNumber = 0;

    void send();
};

#endif //CUBECELL_MONITORING_COMMUNICATION_H

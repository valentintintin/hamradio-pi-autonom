#ifndef CUBECELL_MONITORING_COMMUNICATION_H
#define CUBECELL_MONITORING_COMMUNICATION_H

#include <radio/radio.h>
#include "Config.h"
#include "../lib/Aprs/Aprs.h"
#include "System.h"

class Communication {
public:
    explicit Communication(System* system);

    bool begin(RadioEvents_t *radioEvents);
    void update();
    void sent();
    void received(uint8_t * payload, uint16_t size, int16_t rssi, int8_t snr);

    void sendMessage(const char* message);
    void sendPosition(const char* comment);
    void sendTelemetry(const char* comment, uint8_t uptime, uint8_t vs, uint8_t is, uint8_t vb, uint8_t ib, bool isNight);
private:
    System* system;
    uint8_t buffer[TRX_BUFFER]{};
    char stringBuffer[TRX_BUFFER]{};
    AprsPacket aprsPacketTx, aprsPacketRx;
    uint16_t telemetrySequenceNumber = 0;

    void send();
};

#endif //CUBECELL_MONITORING_COMMUNICATION_H

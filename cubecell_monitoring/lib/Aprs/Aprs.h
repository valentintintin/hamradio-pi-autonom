#ifndef CUBECELL_MONITORING_APRS_H
#define CUBECELL_MONITORING_APRS_H

#define MAX_PACKET_LENGTH 200
#define CALLSIGN_LENGTH 9
#define MAX_PATH 8
#define MAX_TELEMETRY_ANALOG 5
#define MAX_TELEMETRY_BOOLEAN 8
#define TELEMETRY_NAME_LENGTH 7
#define TELEMETRY_UNIT_LENGTH 7
#define TELEMETRY_PROJECT_NAME_LENGTH 23
#define MESSAGE_LENGTH 67
#define ACK_MESSAGE_LENGTH 3

#include <cstdint>

enum AprsPacketType { Position, Message, Telemetry, TelemetryUnit, TelemetryLabel, TelemetryEquation, TelemetryBitSense };

typedef struct {
    // a x value^2 + b x value + c
    double a = 0;
    double b = 0;
    double c = 0;
} AprsTelemetryEquation;

typedef struct {
    char name[TELEMETRY_NAME_LENGTH]{};
    uint8_t value = 0;
    char unit[TELEMETRY_UNIT_LENGTH]{};
    AprsTelemetryEquation equation{};
    bool bitSense = true;
} AprsTelemetry;

typedef struct {
    AprsTelemetry telemetriesAnalog[MAX_TELEMETRY_ANALOG]{};
    AprsTelemetry telemetriesBoolean[MAX_TELEMETRY_BOOLEAN]{};
    uint16_t telemetrySequenceNumber = 0;
    char projectName[TELEMETRY_PROJECT_NAME_LENGTH]{};
} AprsTelemetries;

typedef struct {
    char symbol = '!';
    char overlay = '/';
    double latitude = 0;
    double longitude = 0;
    double courseDeg = 0;
    double speedKnots = 0;
    double altitudeFeet = 0;
} AprsPosition;

typedef struct {
    char destination[CALLSIGN_LENGTH]{};
    char message[MESSAGE_LENGTH]{};
    char ackToConfirm[ACK_MESSAGE_LENGTH]{};
    char ackToReject[ACK_MESSAGE_LENGTH]{};
    char ackToAsk[ACK_MESSAGE_LENGTH]{};
} AprsMessage;

typedef struct {
    char content[MAX_PACKET_LENGTH]{};
    char source[CALLSIGN_LENGTH]{};
    char destination[CALLSIGN_LENGTH]{};
    char path[CALLSIGN_LENGTH * MAX_PATH]{};
    char comment[MESSAGE_LENGTH]{};
    AprsPosition position;
    AprsMessage message;
    AprsTelemetries telemetries;
    AprsPacketType type = Position;
} AprsPacket;

class Aprs {
public:
    static uint8_t encode(AprsPacket* aprsPacket, char* aprsResult);
    static bool decode(const char* aprs, AprsPacket* aprsPacket);
private:
    static void appendPosition(AprsPosition* position, char* aprsResult);
    static void appendTelemetries(AprsPacket *aprsPacket, char* aprsResult);
    static void appendMessage(AprsMessage *message, char* aprsResult);
    static char *ax25Base91Enc(char *s, uint8_t n, uint32_t v);
};

#endif //CUBECELL_MONITORING_APRS_H

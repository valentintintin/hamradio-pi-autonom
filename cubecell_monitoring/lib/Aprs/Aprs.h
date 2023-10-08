#ifndef CUBECELL_MONITORING_APRS_H
#define CUBECELL_MONITORING_APRS_H

#define MAX_PACKET_LENGTH 200
#define CALLSIGN_LENGTH 11
#define MAX_PATH 8
#define MAX_TELEMETRY_ANALOG 5
#define MAX_TELEMETRY_BOOLEAN 8
#define TELEMETRY_NAME_LENGTH 8
#define TELEMETRY_UNIT_LENGTH 8
#define TELEMETRY_PROJECT_NAME_LENGTH 24
#define MESSAGE_LENGTH 68
#define ACK_MESSAGE_LENGTH 4
#define WEATHER_DEVICE_LENGTH 4

#include <cstdint>

enum AprsPacketType { Unknown, Position, Message, Telemetry, TelemetryUnit, TelemetryLabel, TelemetryEquation, TelemetryBitSense, Weather, Item, Object, Status, RawContent };

typedef struct {
    double windDirectionDegress = 0;
    double windSpeedMph = 0;
    double gustSpeedMph = 0;
    double temperatureFahrenheit = 0;
    double rain1HourHundredthsOfAnInch = 0;
    double rain24HourHundredthsOfAnInch = 0;
    double rainSinceMidnightHundredthsOfAnInch = 0;
    double humidity = 0;
    double pressure = 0;
    char device[WEATHER_DEVICE_LENGTH]{};
} AprsWeather;

typedef struct {
    // a x value^2 + b x value + c
    double a = 0;
    double b = 0;
    double c = 0;
} AprsTelemetryEquation;

typedef struct {
    char name[TELEMETRY_NAME_LENGTH]{};
    double value = 0;
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
    bool altitudeInComment = true;
    bool withWeather = false;
    bool withTelemetry = false;
} AprsPosition;

typedef struct {
    char destination[CALLSIGN_LENGTH]{};
    char message[MESSAGE_LENGTH]{};
    char ackToConfirm[ACK_MESSAGE_LENGTH]{}; // when RX
    char ackToReject[ACK_MESSAGE_LENGTH]{}; // when RX
    char ackToAsk[ACK_MESSAGE_LENGTH]{}; // when TX
    char ackConfirmed[ACK_MESSAGE_LENGTH]{}; // when RX after TX
    char ackRejected[ACK_MESSAGE_LENGTH]{}; // when RX after TX
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
    AprsWeather weather;
    AprsPacketType type = Unknown;
} AprsPacket;

class Aprs {
public:
    static uint8_t encode(AprsPacket* aprsPacket, char* aprsResult);
    static bool decode(const char* aprs, AprsPacket* aprsPacket);
    static void reset(AprsPacket* aprsPacket);
private:
    static void appendPosition(AprsPosition* position, char* aprsResult);
    static void appendTelemetries(AprsPacket *aprsPacket, char* aprsResult);
    static void appendMessage(AprsMessage *message, char* aprsResult);
    static void appendWeather(AprsWeather *weather, char* aprsResult);
    static char *ax25Base91Enc(char *destination, uint8_t width, uint32_t value);
    static const char *formatDouble(double value);
    static void trim(char *string);
    static void trimStart(char *string);
    static void trimEnd(char *string);
    static void trimFirstSpace(char *string);
};

#endif //CUBECELL_MONITORING_APRS_H

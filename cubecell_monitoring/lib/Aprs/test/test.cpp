#include <cstring>
#include <cstdio>
#include "../Aprs.h"

int main() {
    AprsPacket packet;
    char encoded_packet[MAX_PACKET_LENGTH];

    strcpy(packet.source, "N0CALL-9");
    strcpy(packet.destination, "APRS");
    strcpy(packet.path, "WIDE1-1,WIDE2-1");
    strcpy(packet.comment, "Comment");
    strcpy(packet.message.message, "Hello, world!");
    strcpy(packet.message.destination, "NOCALL-4");
    packet.position = {
            '!',
            '/',
            40.12345,
            5.12345,
            123,
            45,
            678
    };
    packet.telemetries.telemetrySequenceNumber = 789;
    strcpy(packet.telemetries.projectName, "Test project");
    packet.telemetries.telemetriesAnalog[0] = {
            "Test",
            123,
            "VS"
    };
    packet.telemetries.telemetriesAnalog[1] = {
            "T2",
            45,
            ""
    };
    packet.telemetries.telemetriesAnalog[2] = {
            "",
            45,
            "",
            {
                1, -2, 34.5
            }
    };
    packet.telemetries.telemetriesBoolean[0] = {
            "Bool 1",
            1,
            "on"
    };
    packet.telemetries.telemetriesBoolean[1] = {
            "Bool 2",
            0,
            "oui"
    };
    packet.telemetries.telemetriesBoolean[2] = {
            "",
            123,
            "",
            {},
            false
    };

    packet.type = Position;
    uint8_t size = Aprs::encode(&packet, encoded_packet);
    printf("\nAPRS Position Size : %d\n%s\n", size, encoded_packet);

    packet.type = Message;
    size = Aprs::encode(&packet, encoded_packet);
    printf("\nAPRS Message Size : %d\n%s\n", size, encoded_packet);

    strcpy(packet.message.ackToAsk, "001");
    size = Aprs::encode(&packet, encoded_packet);
    printf("\nAPRS Message ack Size : %d\n%s\n", size, encoded_packet);

    strcpy(packet.message.ackToReject, "016");
    size = Aprs::encode(&packet, encoded_packet);
    printf("\nAPRS Message ack reject Size : %d\n%s\n", size, encoded_packet);

    strcpy(packet.message.ackToConfirm, "009");
    size = Aprs::encode(&packet, encoded_packet);
    printf("\nAPRS Message ack confirm Size : %d\n%s\n", size, encoded_packet);

    strcpy(packet.message.ackToAsk, "010");
    strcpy(packet.message.ackToConfirm, "009");
    size = Aprs::encode(&packet, encoded_packet);
    printf("\nAPRS Message ack and confirm Size : %d\n%s\n", size, encoded_packet);

    packet.type = Telemetry;
    size = Aprs::encode(&packet, encoded_packet);
    printf("\nAPRS Telemetry Size : %d\n%s\n", size, encoded_packet);

    packet.type = TelemetryLabel;
    size = Aprs::encode(&packet, encoded_packet);
    printf("\nAPRS Telemetry label Size : %d\n%s\n", size, encoded_packet);

    packet.type = TelemetryUnit;
    size = Aprs::encode(&packet, encoded_packet);
    printf("\nAPRS Telemetry unit Size : %d\n%s\n", size, encoded_packet);

    packet.type = TelemetryEquation;
    size = Aprs::encode(&packet, encoded_packet);
    printf("\nAPRS Telemetry equations Size : %d\n%s\n", size, encoded_packet);

    packet.type = TelemetryBitSense;
    size = Aprs::encode(&packet, encoded_packet);
    printf("\nAPRS Telemetry bits Size : %d\n%s\n", size, encoded_packet);

    printf("\n");

    AprsPacket packet2;
    if (!Aprs::decode(encoded_packet, &packet2)) {
        printf("Decode error for %s\n\n", encoded_packet);
    } else {
        printf("Received from %s to %s by %s : %s\n\n", packet2.source, packet2.destination, packet2.path, packet2.content);
    }

    return 0;
}
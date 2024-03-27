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
            45, // 83 Km/h
            678 // 206 m
    };
    packet.telemetries.telemetrySequenceNumber = 7544;
    strcpy(packet.telemetries.projectName, "Test project");
    packet.telemetries.telemetriesAnalog[0] = {
            "A1",
            1472,
            "VS"
    };
    packet.telemetries.telemetriesAnalog[1] = {
            "A2",
            1564,
            "",
            {
                0, 0.001, 0
            }
    };
    packet.telemetries.telemetriesAnalog[2] = {
            "",
            -1656.45,
            "",
            {
                1, -2.0987, 34.5
            }
    };
    packet.telemetries.telemetriesAnalog[3] = {
            "",
            1748,
    };
    packet.telemetries.telemetriesAnalog[4] = {
            "",
            1840,
    };
    packet.telemetries.telemetriesBoolean[0] = {
            "Bool1",
            1,
            "on"
    };
    packet.telemetries.telemetriesBoolean[1] = {
            "B2",
            0,
            "yes"
    };
    packet.telemetries.telemetriesBoolean[2] = {
            "",
            0,
            "",
            {},
            false
    };
    packet.telemetries.telemetriesBoolean[2] = {
            "",
            0,
            "",
            {},
            false
    };
    packet.telemetries.telemetriesBoolean[3] = {
            "",
            0,
    };
    packet.telemetries.telemetriesBoolean[4] = {
            "",
            0,
    };
    packet.telemetries.telemetriesBoolean[5] = {
            "",
            0,
    };
    packet.telemetries.telemetriesBoolean[6] = {
            "",
            0,
    };
    packet.telemetries.telemetriesBoolean[7] = {
            "",
            1,
    };

    packet.type = Position;
    uint8_t size = Aprs::encode(&packet, encoded_packet);
    printf("\nAPRS Position Size : %d\n%s\n", size, encoded_packet);

    packet.position.altitudeInComment = false;
    size = Aprs::encode(&packet, encoded_packet);
    printf("\nAPRS Position with altitude compressed Size : %d\n%s\n", size, encoded_packet);

    packet.position.withTelemetry = true;
    size = Aprs::encode(&packet, encoded_packet);
    printf("\nAPRS Position with telemetry Size : %d\n%s\n", size, encoded_packet);

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

    packet.type = Weather;
    packet.weather.windSpeedMph = 5.2;
    packet.weather.windDirectionDegress = 123;
    packet.weather.gustSpeedMph = 20.2;
    packet.weather.temperatureFahrenheit = 23.56;
    packet.weather.humidity = 45.78;
    packet.weather.rain1HourHundredthsOfAnInch = 1.23;
    packet.weather.rain24HourHundredthsOfAnInch = 12.23;
    packet.weather.rainSinceMidnightHundredthsOfAnInch = 6.32;
    packet.weather.pressure = 1123;
    packet.weather.useWindSpeed = true;
    packet.weather.useWindDirection = true;
    packet.weather.useGustSpeed = true;
    packet.weather.useTemperature = true;
    packet.weather.useHumidity = true;
    packet.weather.useRain1Hour = true;
    packet.weather.useRain24Hour = true;
    packet.weather.useRainSinceMidnight = true;
    packet.weather.usePressure = true;
    packet.position.withWeather = true;
    packet.position.withTelemetry = false;
//    strcpy(packet.weather.device, "Test");
    size = Aprs::encode(&packet, encoded_packet);
    printf("\nAPRS Weather Size : %d\n%s\n", size, encoded_packet);

    packet.type = Status;
    strcpy(packet.comment, "I'm good !");
    size = Aprs::encode(&packet, encoded_packet);
    printf("\nAPRS Status size : %d\n%s\n", size, encoded_packet);

    packet.type = RawContent;
    strcpy(packet.content, "Test test test");
    size = Aprs::encode(&packet, encoded_packet);
    printf("\nAPRS Raw Content : %d\n%s\n", size, encoded_packet);

    printf("\n");

    packet.type = Message;
    packet.message.ackToReject[0] = '\0';
    packet.message.ackToAsk[0] = '3';
    packet.message.ackToAsk[1] = '\0';
    packet.message.ackToConfirm[0] = '\0';
    Aprs::encode(&packet, encoded_packet);

    AprsPacketLite packet2;
    if (!Aprs::decode("F4HVV-10>APDR16,WIDE1-1::F4HVV-15 :ack1 hello{2", &packet2)) {
        printf("Decode error for %s\n\n", encoded_packet);
    } else {
        printf("Received type %d from %s to %s by %s --> %s\nMessage length %ld to %s with ack %s and confirmed %s --> %s\n\n",
               packet2.type,
               packet2.source, packet2.destination, packet2.path, packet2.content,
               strlen(packet2.message.message), packet2.message.destination, packet2.message.ackToConfirm, packet2.message.ackConfirmed,
               packet2.message.message);
    }

    AprsPacketLite packet3;
    if (!Aprs::decode("F4HVV-9>APDR16,WIDE1-1:=4519.92N/00537.15E[", &packet3)) {
        printf("Decode error for %s\n\n", encoded_packet);
    } else {
        printf("Received type %d from %s to %s by %s --> %s\n\n",
               packet3.type,
               packet3.source, packet3.destination, packet3.path, packet3.content);
    }

    return 0;
}
#include <cstdio>
#include <cstring>
#include <ctype.h>
#include <math.h>
#include "Aprs.h"

#ifdef NATIVE
#define sprintf_P     sprintf
#define sscanf_P     sscanf
#define strstr_P     strstr
#define strcat_P     strcat
#define PSTR(s)       (s)
#define min         std::min
#define max         std::max
#else
#include "Arduino.h"
#define sscanf_P     sscanf
#endif

uint8_t Aprs::encode(AprsPacket* aprsPacket, char* aprsResult) {
    sprintf_P(aprsResult, PSTR("%s>%s"), aprsPacket->source, aprsPacket->destination);

    if (strlen(aprsPacket->path)) {
        sprintf_P(&aprsResult[strlen(aprsResult)], PSTR(",%s"), aprsPacket->path);
    }

    strcat_P(aprsResult, PSTR(":"));

    switch (aprsPacket->type) {
        case Position:
            appendPosition(&aprsPacket->position, aprsResult);
            break;
        case Message:
            appendMessage(&aprsPacket->message, aprsResult);
            break;
        case Telemetry:
        case TelemetryLabel:
        case TelemetryUnit:
        case TelemetryEquation:
        case TelemetryBitSense:
            appendTelemetries(aprsPacket, aprsResult);
            break;
        default:
            return 0;
    }

    if (strlen(aprsPacket->comment) && (aprsPacket->type == Position || aprsPacket->type == Telemetry)) {
        sprintf_P(&aprsResult[strlen(aprsResult)], PSTR(" %s"), aprsPacket->comment); // Normally, 40 max for position
    }

    if (aprsPacket->type == Position && aprsPacket->position.withTelemetry) {
        appendTelemetries(aprsPacket, aprsResult);
    }

    trim(aprsResult);

    return strlen(aprsResult);
}

bool Aprs::decode(const char* aprs, AprsPacket* aprsPacket) {
    if (sscanf_P(aprs, PSTR("%9[^>]>%9[^,],%19[^:]:"), aprsPacket->source, aprsPacket->destination, aprsPacket->path) != 3) {
        return false;
    }

    char *point = strchr((char*) aprs, ':');
    if (point != nullptr) {
        strcpy(aprsPacket->content, point + 1);

        trim(aprsPacket->content);

        if (aprsPacket->content[0] == ':') {
            strcpy(aprsPacket->message.message, aprsPacket->content + 1);

            char* find = strchr(aprsPacket->message.message, ':');
            if (find != nullptr) {
                strncpy(aprsPacket->message.destination, aprsPacket->message.message, find - aprsPacket->message.message);
                strcpy(aprsPacket->message.message, find + 1);
            }

            find = strstr_P(aprsPacket->message.message, PSTR("ack"));
            if (find != nullptr) {
                strcpy(aprsPacket->message.ackConfirmed, find + 3);
                trimFirstSpace(aprsPacket->message.ackConfirmed);
                strcpy(aprsPacket->message.message, find + 3 + strlen(aprsPacket->message.ackConfirmed));
            }

            find = strstr_P(aprsPacket->message.message, PSTR("rej"));
            if (find != nullptr) {
                strcpy(aprsPacket->message.ackRejected, find + 3);
                trimFirstSpace(aprsPacket->message.ackRejected);
                strcpy(aprsPacket->message.message, find + 3 + strlen(aprsPacket->message.ackRejected));
            }

            find = strchr(aprsPacket->message.message, '{');
            if (find != nullptr) {
                strcpy(aprsPacket->message.ackToConfirm, find + 1);
                aprsPacket->message.message[find - aprsPacket->message.message] = '\0'; // remove '{'
            }

            find = strstr_P(aprsPacket->message.message, PSTR("BITS"));
            if (find != nullptr) {
                aprsPacket->type = TelemetryBitSense;

                find = strchr(aprsPacket->message.message, ',');
                if (find != nullptr) {
                    strcpy(aprsPacket->telemetries.projectName, find + 1);
                }
            }

            find = strstr_P(aprsPacket->message.message, PSTR("EQNS"));
            if (find != nullptr) {
                aprsPacket->type = TelemetryEquation;
            }

            find = strstr_P(aprsPacket->message.message, PSTR("PARM"));
            if (find != nullptr) {
                aprsPacket->type = TelemetryLabel;
            }

            find = strstr_P(aprsPacket->message.message, PSTR("UNIT"));
            if (find != nullptr) {
                aprsPacket->type = TelemetryUnit;
            }

            find = strstr_P(aprsPacket->message.message, PSTR("T#"));
            if (find != nullptr) {
                aprsPacket->type = Telemetry;
            }

            trim(aprsPacket->message.message);
        }
    }

    return true;
}

void Aprs::appendPosition(AprsPosition* position, char* aprsResult) {
    uint32_t latitude, longitude;
    latitude = 900000000 - position->latitude * 10000000;
    latitude = latitude / 26 - latitude / 2710 + latitude / 15384615;
    longitude = 900000000 + position->longitude * 10000000 / 2;
    longitude = longitude / 26 - longitude / 2710 + longitude / 15384615;

    sprintf_P(&aprsResult[strlen(aprsResult)], PSTR("!%c"), position->overlay);

    char bufferBase91[] = {"0000\0" };
    ax25Base91Enc(bufferBase91, 4, latitude);
    strcat(aprsResult, reinterpret_cast<const char *>(bufferBase91));

    ax25Base91Enc(bufferBase91, 4, longitude);
    strcat(aprsResult, reinterpret_cast<const char *>(bufferBase91));

    strncat(aprsResult, &position->symbol, 1);

    ax25Base91Enc(bufferBase91, 1, (uint32_t) position->courseDeg / 4);
    strncat(aprsResult, &bufferBase91[0], 1);
    ax25Base91Enc(bufferBase91, 1, (uint32_t) (log1p(position->speedKnots) / 0.07696));
    strncat(aprsResult, &bufferBase91[0], 1);

    strcat_P(aprsResult, PSTR("G"));

    if (position->altitudeFeet > 0) {
        int alt_int = max(-99999, min(999999, (int) position->altitudeFeet));
        if (alt_int < 0) {
            sprintf_P(&aprsResult[strlen(aprsResult)], PSTR("/A=-%05d"), alt_int * -1);
        } else {
            sprintf_P(&aprsResult[strlen(aprsResult)], PSTR("/A=%06d"), alt_int);
        }
    }
}

void Aprs::appendTelemetries(AprsPacket *aprsPacket, char* aprsResult) {
    if (aprsPacket->telemetries.telemetrySequenceNumber > 999) {
        aprsPacket->telemetries.telemetrySequenceNumber = 0;
    }

    char bufferBase91[] = {"00\0"};

    switch (aprsPacket->type) {
        case Position:
            strcat_P(aprsResult, PSTR("|"));

            ax25Base91Enc(bufferBase91, 2, aprsPacket->telemetries.telemetrySequenceNumber++);
            strcpy(&aprsResult[strlen(aprsResult)], bufferBase91);

            for (auto telemetry : aprsPacket->telemetries.telemetriesAnalog) {
                ax25Base91Enc(bufferBase91, 2, abs(telemetry.value)); // Not negative value in compressed mode
                strcpy(&aprsResult[strlen(aprsResult)], bufferBase91);
            }

            strcat_P(aprsResult, PSTR("|"));
            break;
        case Telemetry:
            sprintf_P(&aprsResult[strlen(aprsResult)], PSTR("T#%d,"), aprsPacket->telemetries.telemetrySequenceNumber++);

            for (auto telemetry : aprsPacket->telemetries.telemetriesAnalog) {
                sprintf_P(&aprsResult[strlen(aprsResult)], formatDouble(telemetry.value), telemetry.value);
                strcat_P(aprsResult, PSTR(","));
            }

            for (auto telemetry : aprsPacket->telemetries.telemetriesBoolean) {
                sprintf_P(&aprsResult[strlen(aprsResult)], PSTR("%d"), telemetry.value > 0 ? 1 : 0);
            }
            break;
        case TelemetryLabel:
            sprintf_P(&aprsResult[strlen(aprsResult)], PSTR(":%-9s:PARM.%-1.7s,%-1.6s,%-1.5s,%-1.5s,%-1.4s,%-1.5s,%-1.4s,%-1.4s,%-1.4s,%-1.4s,%-1.3s,%-1.3s,%-1.3s"),
                      aprsPacket->source,
                      aprsPacket->telemetries.telemetriesAnalog[0].name,
                      aprsPacket->telemetries.telemetriesAnalog[1].name,
                      aprsPacket->telemetries.telemetriesAnalog[2].name,
                      aprsPacket->telemetries.telemetriesAnalog[3].name,
                      aprsPacket->telemetries.telemetriesAnalog[4].name,
                      aprsPacket->telemetries.telemetriesBoolean[0].name,
                      aprsPacket->telemetries.telemetriesBoolean[1].name,
                      aprsPacket->telemetries.telemetriesBoolean[2].name,
                      aprsPacket->telemetries.telemetriesBoolean[3].name,
                      aprsPacket->telemetries.telemetriesBoolean[4].name,
                      aprsPacket->telemetries.telemetriesBoolean[5].name,
                      aprsPacket->telemetries.telemetriesBoolean[6].name,
                      aprsPacket->telemetries.telemetriesBoolean[7].name
            );
            break;
        case TelemetryUnit:
            sprintf_P(&aprsResult[strlen(aprsResult)], PSTR(":%-9s:UNIT.%-1.7s,%-1.6s,%-1.5s,%-1.5s,%-1.4s,%-1.5s,%-1.4s,%-1.4s,%-1.4s,%-1.4s,%-1.3s,%-1.3s,%-1.3s"),
                      aprsPacket->source,
                      aprsPacket->telemetries.telemetriesAnalog[0].unit,
                      aprsPacket->telemetries.telemetriesAnalog[1].unit,
                      aprsPacket->telemetries.telemetriesAnalog[2].unit,
                      aprsPacket->telemetries.telemetriesAnalog[3].unit,
                      aprsPacket->telemetries.telemetriesAnalog[4].unit,
                      aprsPacket->telemetries.telemetriesBoolean[0].unit,
                      aprsPacket->telemetries.telemetriesBoolean[1].unit,
                      aprsPacket->telemetries.telemetriesBoolean[2].unit,
                      aprsPacket->telemetries.telemetriesBoolean[3].unit,
                      aprsPacket->telemetries.telemetriesBoolean[4].unit,
                      aprsPacket->telemetries.telemetriesBoolean[5].unit,
                      aprsPacket->telemetries.telemetriesBoolean[6].unit,
                      aprsPacket->telemetries.telemetriesBoolean[7].unit
            );
            break;
        case TelemetryEquation:
            sprintf_P(&aprsResult[strlen(aprsResult)], PSTR(":%-9s:EQNS."), aprsPacket->source);

            for (uint8_t i = 0; i < MAX_TELEMETRY_ANALOG; i++) {
                AprsTelemetryEquation equation = aprsPacket->telemetries.telemetriesAnalog[i].equation;

                if (i > 0) {
                    strcat_P(aprsResult, PSTR(","));
                }

                sprintf_P(&aprsResult[strlen(aprsResult)], formatDouble(equation.a), equation.a);
                strcat_P(aprsResult, PSTR(","));
                sprintf_P(&aprsResult[strlen(aprsResult)], formatDouble(equation.b), equation.b);
                strcat_P(aprsResult, PSTR(","));
                sprintf_P(&aprsResult[strlen(aprsResult)], formatDouble(equation.c), equation.c);
            }

            break;
        case TelemetryBitSense:
            sprintf_P(&aprsResult[strlen(aprsResult)], PSTR(":%-9s:BITS."), aprsPacket->source);

            for (auto telemetry : aprsPacket->telemetries.telemetriesBoolean) {
                strcat_P(aprsResult, telemetry.bitSense ? PSTR("1") : PSTR("0"));
            }

            if (strlen(aprsPacket->telemetries.projectName)) {
                sprintf_P(&aprsResult[strlen(aprsResult)], PSTR(",%.23s"), aprsPacket->telemetries.projectName);
            }
            break;
        default:
            return;
    }
}

void Aprs::appendMessage(AprsMessage *message, char* aprsResult) {
    sprintf_P(&aprsResult[strlen(aprsResult)], PSTR(":%-9s:"), message->destination);

    if (strlen(message->ackToReject)) {
        sprintf_P(&aprsResult[strlen(aprsResult)], PSTR("rej%s "), message->ackToReject);
    } else if (strlen(message->ackToConfirm)) {
        sprintf_P(&aprsResult[strlen(aprsResult)], PSTR("ack%s "), message->ackToConfirm);
    }

    if (strlen(message->message)) {
        strcat(aprsResult, message->message);
    }

    if (strlen(message->ackToAsk)) {
        sprintf_P(&aprsResult[strlen(aprsResult)], PSTR("{%s"), message->ackToAsk);
    }
}

char* Aprs::ax25Base91Enc(char *destination, uint8_t width, uint32_t value) {
    /* Creates a Base-91 representation of the value in v in the string */
    /* pointed to by s, n-characters long. String length should be n+1. */

    for(destination += width, *destination = '\0'; width; width--) {
        *(--destination) = value % 91 + 33;
        value /= 91;
    }

    return(destination);
}

const char *Aprs::formatDouble(double value) {
    double integral;
    double fractional = modf(value, &integral);

    if (fractional != 0) {
        return PSTR("%.2f");
    }

    return PSTR("%.0f");
}

void Aprs::trim(char *string) {
    trimStart(string);
    trimEnd(string);
}

void Aprs::trimEnd(char *string) {
    string[strcspn(string, "\n")] = '\0';
    size_t len = strlen(string);
    while (len > 0 && (string[len - 1] == ' ' || string[len - 1] == '\n')) {
        string[--len] = '\0';
    }
}

void Aprs::trimStart(char *string) {
    size_t len = strlen(string);
    int start = 0;

    while (isspace((unsigned char)string[start])) {
        start++;
    }

    if (start > 0) {
        memmove(string, string + start, len - start + 1);
    }
}

void Aprs::trimFirstSpace(char *string) {
    string[strcspn(string, " ")] = '\0';
}

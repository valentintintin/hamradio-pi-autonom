#include <cstdio>
#include <cstring>
#include <math.h>
#include "Aprs.h"

#ifdef NATIVE
#define sprintf_P     sprintf
#define sscanf_P     sscanf
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
        sprintf_P(&aprsResult[strlen(aprsResult)], PSTR(" %s"), aprsPacket->comment);
    }

    return strlen(aprsResult);
}

bool Aprs::decode(const char* aprs, AprsPacket* aprsPacket) {
    if (sscanf_P(aprs, PSTR("%9[^>]>%9[^,],%19[^:]:"), aprsPacket->source, aprsPacket->destination, aprsPacket->path) != 3) {
        return false;
    }

    char *point = strchr((char*) aprs, ':');
    if (point != nullptr) {
        strcpy(aprsPacket->content, point + 1);

        char* point = strchr(aprsPacket->content, '{');
        if (point != nullptr) {
            strcpy(aprsPacket->message.ackToConfirm, point + 1);
        }
    }

    return true;
}

void Aprs::appendPosition(AprsPosition* position, char* aprsResult) {
    uint32_t aprs_lat, aprs_lon;
    aprs_lat = 900000000 - position->latitude * 10000000;
    aprs_lat = aprs_lat / 26 - aprs_lat / 2710 + aprs_lat / 15384615;
    aprs_lon = 900000000 + position->longitude * 10000000 / 2;
    aprs_lon = aprs_lon / 26 - aprs_lon / 2710 + aprs_lon / 15384615;

    sprintf_P(&aprsResult[strlen(aprsResult)], PSTR("!%c"), position->overlay);

    char helper_base91[] = { "0000\0" };
    ax25Base91Enc(helper_base91, 4, aprs_lat);
    strcat(aprsResult, reinterpret_cast<const char *>(helper_base91));

    ax25Base91Enc(helper_base91, 4, aprs_lon);
    strcat(aprsResult, reinterpret_cast<const char *>(helper_base91));

    strncat(aprsResult, &position->symbol, 1);

    ax25Base91Enc(helper_base91, 1, (uint32_t) position->courseDeg / 4);
    strncat(aprsResult, &helper_base91[0], 1);
    ax25Base91Enc(helper_base91, 1, (uint32_t) (log1p(position->speedKnots) / 0.07696));
    strncat(aprsResult, &helper_base91[0], 1);

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
    switch (aprsPacket->type) {
        case Telemetry:
            if (aprsPacket->telemetries.telemetrySequenceNumber > 999) {
                aprsPacket->telemetries.telemetrySequenceNumber = 0;
            }

            sprintf_P(&aprsResult[strlen(aprsResult)], PSTR("T#%d,"), aprsPacket->telemetries.telemetrySequenceNumber);

            for (auto telemetry : aprsPacket->telemetries.telemetriesAnalog) {
                sprintf_P(&aprsResult[strlen(aprsResult)], PSTR("%03d,"), telemetry.value);
            }

            for (auto telemetry : aprsPacket->telemetries.telemetriesBoolean) {
                sprintf_P(&aprsResult[strlen(aprsResult)], PSTR("%d"), telemetry.value > 0 ? 1 : 0);
            }
            break;
        case TelemetryLabel:
            sprintf_P(&aprsResult[strlen(aprsResult)], PSTR(":%-9s:PARM."), aprsPacket->source);

            for (uint8_t i = 0; i < MAX_TELEMETRY_ANALOG; i++) {
                AprsTelemetry telemetry = aprsPacket->telemetries.telemetriesAnalog[i];

                if (i > 0) {
                    strcat_P(aprsResult, PSTR(","));
                }

                strcat(aprsResult, telemetry.name);
            }

            for (auto telemetry : aprsPacket->telemetries.telemetriesBoolean) {
                strcat(aprsResult, telemetry.name);
            }
            break;
        case TelemetryUnit:
            sprintf_P(&aprsResult[strlen(aprsResult)], PSTR(":%-9s:UNIT."), aprsPacket->source);

            for (uint8_t i = 0; i < MAX_TELEMETRY_ANALOG; i++) {
                AprsTelemetry telemetry = aprsPacket->telemetries.telemetriesAnalog[i];

                if (i > 0) {
                    strcat_P(aprsResult, PSTR(","));
                }

                strcat(aprsResult, telemetry.unit);
            }

            for (uint8_t i = 0; i < MAX_TELEMETRY_BOOLEAN; i++) {
                AprsTelemetry telemetry = aprsPacket->telemetries.telemetriesBoolean[i];

                if (i > 0) {
                    strcat_P(aprsResult, PSTR(","));
                }

                strcat(aprsResult, telemetry.unit);
            }
            break;
        case TelemetryEquation:
            sprintf_P(&aprsResult[strlen(aprsResult)], PSTR(":%-9s:EQNS."), aprsPacket->source);

            for (uint8_t i = 0; i < MAX_TELEMETRY_ANALOG; i++) {
                AprsTelemetry telemetry = aprsPacket->telemetries.telemetriesAnalog[i];

                if (i > 0) {
                    strcat_P(aprsResult, PSTR(","));
                }

                sprintf_P(&aprsResult[strlen(aprsResult)], PSTR("%.2f,%.2f,%.2f"), telemetry.equation.a, telemetry.equation.b, telemetry.equation.c);
            }
            break;
        case TelemetryBitSense:
            sprintf_P(&aprsResult[strlen(aprsResult)], PSTR(":%-9s:BITS."), aprsPacket->source);

            for (auto telemetry : aprsPacket->telemetries.telemetriesBoolean) {
                strcat_P(aprsResult, telemetry.bitSense ? PSTR("1") : PSTR("0"));
            }

            sprintf_P(&aprsResult[strlen(aprsResult)], PSTR(",%s"), aprsPacket->telemetries.projectName);
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

char* Aprs::ax25Base91Enc(char *s, uint8_t n, uint32_t v) {
    /* Creates a Base-91 representation of the value in v in the string */
    /* pointed to by s, n-characters long. String length should be n+1. */

    for(s += n, *s = '\0'; n; n--) {
        *(--s) = v % 91 + 33;
        v /= 91;
    }

    return(s);
}
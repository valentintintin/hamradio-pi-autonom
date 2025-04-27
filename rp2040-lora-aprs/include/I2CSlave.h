#ifndef RP2040_LORA_APRS_I2CSLAVE_H
#define RP2040_LORA_APRS_I2CSLAVE_H

#include <config.h>
#include <Wire.h>

#define REG_BATTERY_VOLTAGE 0x0
#define REG_BATTERY_CURRENT 0x1
#define REG_SOLAR_VOLTAGE 0x2
#define REG_SOLAR_CURRENT 0x3
#define REG_TEMPERATURE 0x4
#define REG_PRESSURE 0x5
#define REG_HUMIDITY 0x6
#define REG_SECONDS 0x7
#define REG_MINUTES 0x8
#define REG_HOURS 0x9
#define REG_DAYS 0xA
#define REG_MONTHS 0xB
#define REG_YEARS 0xC

#define REG_PING 0x1A

#define REG_COMMAND_RECEIVE_FROM_SLAVE 0x20
#define REG_COMMAND_RESPONSE_TRANSMIT_FROM_MASTER 0x21
#define REG_COMMAND_RECEIVED_FROM_MASTER 0x22
#define REG_COMMAND_RESPONSE_TO_MASTER 0x23

#define HAS_POWER 0b1
#define HAS_ENVIRONMENT 0b10
#define HAS_DATETIME 0b100

class System;

class I2CSlave {
public:
    static void begin(System *system);
    static void end();
    static void sendCommandToMaster(const char* command);

    static char commandResponseFromMaster[BUFFER_LENGTH + 1];
    static char commandReceivedFromMaster[BUFFER_LENGTH + 1];
private:
    static System *system;

    static uint8_t currentRegToRead;
    static char commandToSendToMaster[BUFFER_LENGTH + 1];
    static char commandResponseToSendToMaster[BUFFER_LENGTH + 1];

    static void onRequest();
    static void onReceive(int bytes);
};


#endif //RP2040_LORA_APRS_I2CSLAVE_H

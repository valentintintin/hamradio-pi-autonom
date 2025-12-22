#pragma once

#include <Arduino.h>

#include "config.h"

#define NB_SETTINGS 82

enum SettingsType
{
    Boolean, Int8, Int16, Int32, Int64, UInt8, UInt16, UInt32, UInt64, Char, Float, Double, CharString
};

// typedef struct {
//     float frequency = 433.775;
//     uint16_t bandwidth = 125;
//     uint8_t spreadingFactor = 12;
//     uint8_t codingRate = 5;
//     uint8_t outputPower = 22;
//     uint8_t syncWord = 0x12;
//     bool txEnabled = true;
//     bool boostedRxGain = true;
//     bool watchdogTxEnabled = true;
//     uint64_t intervalTimeoutWatchdogTx = 7200000;
// } SettingsLoRa;
//
// typedef struct {
//     char callsign[CALLSIGN_LENGTH + 1]{};
//     char destination[CALLSIGN_LENGTH + 1]{};
//     char path[CALLSIGN_LENGTH * MAX_PATH + 1]{};
//     char pathTelemetry[CALLSIGN_LENGTH + 1]{};
//     char positionComment[MESSAGE_LENGTH + 1]{};
//     char status[MESSAGE_LENGTH + 1]{};
//     char symbol{};
//     char symbolTable{};
//     double latitude = 0;
//     double longitude = 0;
//     uint16_t altitude = 0;
//     bool digipeaterEnabled = true;
//     bool telemetryEnabled = true;
//     uint64_t intervalTelemetry = 900000;
//     bool statusEnabled = true;
//     uint64_t intervalStatus = 86400000;
//     bool positionWeatherEnabled = true;
//     uint64_t intervalPositionWeather = 3600000;
//     bool telemetryInPosition = false;
//     uint16_t telemetrySequenceNumber = 0;
// } SettingsAprs;

// typedef struct {
//     bool enabled = false;
//     uint8_t timeout = 90;
//     uint64_t intervalFeed = 30000;
//     uint16_t timeOff = 30000;
// } SettingsMpptWatchdog;
//
// typedef struct {
//     bool enabled = true;
//     uint64_t intervalCheck = 60000;
//     bool decodeWH65B = false;
//     uint64_t intervalWH65B = 300000;
// } SettingsWeather;

// enum TypeEnergySensor { dummy, mpptchg, ina, adc };

// typedef struct {
//     uint64_t intervalCheck = 30000;
//     TypeEnergySensor type = dummy;
//     uint8_t adcPin = 26;
//     ina3221_ch_t inaChannelBattery = INA3221_CH1;
//     ina3221_ch_t inaChannelSolar = INA3221_CH2;
//     uint16_t mpptPowerOnVoltage = 12000;
//     uint16_t mpptPowerOffVoltage = 11550;
//     bool sendAprsMessageWhenAlert = false;
//     char callsignToSendMessageAlert[CALLSIGN_LENGTH + 1]{};
// } SettingsEnergy;

// typedef struct {
// bool enabled = true;
// uint8_t address = 0x11;
// } SettingsI2CSlave;

typedef struct
{
    pin_size_t pin = 0;
    PinMode mode = OUTPUT;
    bool inverted = false;
    char name[16 + 1]{};
    uint8_t i2cAddress = 0;
} SettingsPin;

// typedef struct {
//     bool enabled = false;
//     uint64_t intervalTimeoutWatchdog = 300000;
//     SettingsPin pin{};
//     bool aprsSendItemEnabled = false;
//     uint64_t intervalSendItem = 3600000;
//     char itemName[TELEMETRY_NAME_LENGTH + 1]{};
//     char itemComment[MESSAGE_LENGTH + 1]{};
//     char symbol{};
//     char symbolTable{};
//     double latitude = 0;
//     double longitude = 0;
//     uint16_t altitude = 0;
// } SettingsWatchdogAndAprsItem;

typedef struct
{
    bool enabled = false;
} SettingsRtc;

typedef struct
{
    uint16_t version = SETTINGS_VERSION;
    bool useSlowClock = false;

    SettingsPin pins[MAX_GPIO_USED]{};
    // SettingsLoRa lora{};
    // SettingsAprs aprs{};
    // SettingsEnergy energy{};
    // SettingsWeather weather{};
    // SettingsMpptWatchdog mpptWatchdog{};
    // SettingsI2CSlave i2c{};
    // SettingsWatchdogAndAprsItem meshtastic{};
    // SettingsWatchdogAndAprsItem linux{};
} Settings;

// typedef struct {
// char callsign[CALLSIGN_LENGTH + 1]{};
// time_t time = 0;
// float rssi = 0;
// float snr = 0;
// char content[MAX_PACKET_LENGTH + 1]{};
// uint64_t count = 0;
// char digipeaterCallsign[CALLSIGN_LENGTH + 1]{};
// uint8_t digipeaterCount = 0;
// } SettingsAprsCallsignHeard;

typedef struct
{
    char name[32 + 1]{};
    SettingsType type = UInt8;
    void* pointer{};
    uint32_t maxSize = 1;
    size_t parentSize = 0;
    uint32_t maxStringLength = 0;
} SettingsGetSetFunction;

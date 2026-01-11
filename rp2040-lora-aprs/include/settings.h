#pragma once

#include <Arduino.h>

#define MAX_GPIO_USED 32
#define MAX_LORA_MODEM 4
#define SETTINGS_VERSION 1
#define NAME_SETTING_LENGTH 32

enum SettingsType
{
    Boolean, Int8, Int16, Int32, Int64, UInt8, UInt16, UInt32, UInt64, Char, Float, Double, CharString
};

enum SettingLoRaMode
{
    LoRaModeAprs, LoRaModeMeshcore, LoRaModeMeshtasticLM, LoRaModeMeshtasticLF
};

struct SettingsLoRaModem {
    SettingLoRaMode mode;
    bool enabled = false;
    float frequency = 434;
    float bandwidth = 125;
    uint8_t spreadingFactor = 9;
    uint8_t codingRate = 7;
    uint8_t syncWord = 0x12;
    uint8_t preambleLength = 8;
};

struct SettingsLoRa
{
    bool txEnabled = true;
    uint8_t outputPower = 22;

    // TODO watchdog RX Lora
    bool watchdogRxEnabled = true;
    uint32_t intervalTimeoutWatchdogRx = 7200;

    SettingLoRaMode mode = LoRaModeAprs;
    uint32_t intervalLoopMode = 0;

    SettingsLoRaModem modems[MAX_LORA_MODEM]{};
};

//
// struct {
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

struct SettingsMpptWatchdog {
    bool enabled = false;
    uint8_t timeout = 255;
    uint8_t intervalFeed = 30;
    uint16_t timeOff = 10;
};
//
// struct {
//     bool enabled = true;
//     uint64_t intervalCheck = 60000;
//     bool decodeWH65B = false;
//     uint64_t intervalWH65B = 300000;
// } SettingsWeather;

struct SettingsMpptCharger
{
    uint16_t powerOnVoltage = 12000;
    uint16_t powerOffVoltage = 11550;
    SettingsMpptWatchdog watchdog{};
};

struct SettingsGpio
{
    bool enabled = false;
    pin_size_t pin = 0;
    pin_size_t pinToLow = 0;
    PinMode mode = OUTPUT;
    bool inverted = false;
    uint8_t i2cAddress = 0;
};

// struct {
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

struct Settings
{
    uint16_t version = SETTINGS_VERSION;


    bool useWatchdog = true;
    bool useSlowClock = false;
    bool i2cSlaveEnabled = true;

    SettingsGpio gpio[MAX_GPIO_USED]{};
    SettingsMpptCharger mppt{};
    SettingsLoRa lora{}; // TODO setter et getter
};

// struct {
// char callsign[CALLSIGN_LENGTH + 1]{};
// time_t time = 0;
// float rssi = 0;
// float snr = 0;
// char content[MAX_PACKET_LENGTH + 1]{};
// uint64_t count = 0;
// char digipeaterCallsign[CALLSIGN_LENGTH + 1]{};
// uint8_t digipeaterCount = 0;
// } SettingsAprsCallsignHeard;

struct SettingsGetSetFunction
{
    char name[NAME_SETTING_LENGTH + 1]{};
    SettingsType type = UInt8;
    void* pointer{};
    uint32_t maxSize = 1;
    size_t parentSize = 0;
    uint32_t maxStringLength = 0;
};

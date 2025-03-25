#ifndef RP2040_LORA_APRS_SETTINGS_H
#define RP2040_LORA_APRS_SETTINGS_H

#include <Arduino.h>
#include "Aprs.h"
#include "INA3221.h"

#define APRS_CALLSIGNS_HEARD_NUMBER 30

typedef struct {
    float frequency;
    uint16_t bandwidth;
    uint8_t spreadingFactor;
    uint8_t codingRate;
    uint8_t outputPower;
    bool txEnabled;
    bool watchdogTxEnabled;
    uint64_t intervalTimeoutWatchdogTx;

    uint8_t reserved[8];
} SettingsLoRa;

typedef struct {
    char call[CALLSIGN_LENGTH];
    char destination[CALLSIGN_LENGTH];
    char path[CALLSIGN_LENGTH * MAX_PATH];
    char comment[MESSAGE_LENGTH];
    char status[MESSAGE_LENGTH];
    char symbol;
    char symbolTable;
    double latitude;
    double longitude;
    uint16_t altitude;
    bool digipeaterEnabled;
    bool telemetryEnabled;
    uint64_t intervalTelemetry;
    bool statusEnabled;
    uint64_t intervalStatus;
    bool positionWeatherEnabled;
    uint64_t intervalPositionWeather;
    bool telemetryInPosition;
    uint16_t telemetrySequenceNumber;

    uint8_t reserved[128];
} SettingsAprs;

typedef struct {
    bool enabled;
    uint64_t timeout;
    uint64_t intervalFeed;
    uint16_t timeOff;

    uint8_t reserved[8];
} SettingsMpptWatchdog;

typedef struct {
    bool enabled;
    uint64_t intervalCheck;
    uint8_t pin;

    uint8_t reserved[8];
} SettingsBoxOpened;

typedef struct {
    bool enabled;
    uint64_t intervalCheck;

    uint8_t reserved[8];
} SettingsWeather;

enum TypeEnergySensor { dummy, mpptchg, ina, adc };

typedef struct {
    uint64_t intervalCheck;
    TypeEnergySensor type;
    uint8_t adcPin;
    ina3221_ch_t inaChannelBattery;
    ina3221_ch_t inaChannelSolar;
    uint16_t mpptPowerOnVoltage;
    uint16_t mpptPowerOffVoltage;
    bool sendAprsMessageWhenAlert;

    uint8_t reserved[8];
} SettingsEnergy;

typedef struct {
    bool watchdogEnabled;
    uint64_t intervalTimeoutWatchdog;
    uint8_t pin;
    bool i2cSlaveEnabled;
    uint8_t i2cSlaveAddress;
    bool aprsSendItemEnabled;
    uint64_t intervalSendItem;
    char itemName[TELEMETRY_NAME_LENGTH];
    char itemComment[MESSAGE_LENGTH];
    char symbol;
    char symbolTable;
    double latitude;
    double longitude;
    uint16_t altitude;

    uint8_t reserved[128];
} SettingsMeshtastic;

typedef struct {
    bool watchdogEnabled;
    uint64_t intervalTimeoutWatchdog;
    uint8_t pin;
    uint8_t nprPin;
    uint8_t wifiPin;
    bool aprsSendItemEnabled;
    uint64_t intervalSendItem;
    char itemName[TELEMETRY_NAME_LENGTH];
    char itemComment[MESSAGE_LENGTH];
    char symbol;
    char symbolTable;
    double latitude;
    double longitude;
    uint16_t altitude;

    uint8_t reserved[128];
} SettingsLinux;

typedef struct {
    bool enabled;
    uint8_t wakeUpPin;

    uint8_t reserved[8];
} SettingsRtc;

typedef struct {
    char callsign[CALLSIGN_LENGTH];
    time_t time;
    float rssi;
    float snr;
    char content[MAX_PACKET_LENGTH];
    uint64_t count;
    char digipeaterCallsign[CALLSIGN_LENGTH];
    uint8_t digipeaterCount;

    uint8_t reserved[128];
} SettingsAprsCallsignHeard;

typedef struct {
    SettingsLoRa lora;
    SettingsEnergy energy;
    SettingsAprs aprs;
    SettingsMeshtastic meshtastic;
    SettingsMpptWatchdog mpptWatchdog;
    SettingsWeather weather;
    SettingsBoxOpened boxOpened;
    SettingsLinux linux;
    SettingsRtc rtc;
    bool useInternalWatchdog;
    bool useSlowClock;
    SettingsAprsCallsignHeard aprsCallsignsHeard[APRS_CALLSIGNS_HEARD_NUMBER];

    uint8_t reserved[512];
} Settings;

#endif //RP2040_LORA_APRS_SETTINGS_H

#ifndef CUBECELL_MONITORING_CONFIG_H
#define CUBECELL_MONITORING_CONFIG_H

#include <JsonWriter.h>

#define LOG_LEVEL LOG_LEVEL_TRACE
#define WireUsed Wire
#define SerialPiUsed Serial1

#define USE_RF true
#define USE_RTC true
#define SET_RTC 0

#define RF_FREQUENCY 433775000 // Hz
#define LORA_BANDWIDTH 0 // [0: 125 kHz,
//  1: 250 kHz,
//  2: 500 kHz,
//  3: Reserved]
#define LORA_SPREADING_FACTOR 12 // [SF7..SF12]
#define LORA_CODINGRATE 1 // [1: 4/5,
//  2: 4/6,
//  3: 4/7,
//  4: 4/8]
#define LORA_PREAMBLE_LENGTH 8 // Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT 0 // Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON false
#define LORA_IQ_INVERSION_ON false
#define TX_OUTPUT_POWER 20
#define TRX_BUFFER 210

#define INTERVAL_TELEMETRY_APRS 300000 // 5 minutes
#define INTERVAL_POSITION_APRS 750000 // 15 minutes
#define INTERVAL_ALARM_BOX_OPENED_APRS 120000 // 2 minutes

#define INTERVAL_WEATHER 10000
#define INTERVAL_MPPT 10000
#define INTERVAL_TIME 30000
#define TIME_PAUSE_SCREEN 1500
#define TIME_SCREEN_ON 20000

#define APRS_CALLSIGN "F4HVV-15"
#define APRS_PATH "WIDE1-1"
#define APRS_PATH_MESSAGE "WIDE1-1"
#define APRS_DESTINATION "F4HVV"
#define APRS_SYMBOL 'I'
#define APRS_SYMBOL_TABLE '/'
#define APRS_LATITUDE 45.3283542
#define APRS_LONGITUDE 5.6344881
#define APRS_ALTITUDE 820
#define APRS_COMMENT "f4hvv.valentin-saugnier.fr"

#define PIN_DHT GPIO11
#define PIN_WIFI GPIO12
#define PIN_NPR GPIO14
#define PIN_LDR ADC2

#define WATCHDOG_TIMEOUT 30
#define LOW_VOLTAGE 11400
#define LDR_ALARM_LEVEL 1000

extern JsonWriter serialJsonWriter;

#endif //CUBECELL_MONITORING_CONFIG_H

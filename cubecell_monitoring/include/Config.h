#ifndef CUBECELL_MONITORING_CONFIG_H
#define CUBECELL_MONITORING_CONFIG_H

#include <softSerial.h>
#include <JsonWriter.h>

#define LOG_LEVEL LOG_LEVEL_TRACE

//#define USE_SCREEN
#define USE_SOFT_SERIAL
#define USE_RF false
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
#define INTERVAL_POSITION_APRS 300000 // 5 minutes
#define INTERVAL_STATUS_APRS 3600000 // 60 minutes
#define INTERVAL_ALARM_BOX_OPENED_APRS 120000 // 2 minutes

#define INTERVAL_WEATHER 30000
#define INTERVAL_MPPT 30000
#define INTERVAL_TIME 30000
#define TIME_PAUSE_SCREEN 1500
#define TIME_SCREEN_ON 30000

#define APRS_CALLSIGN "F4HVV-15"
#define APRS_PATH "WIDE1-1"
#define APRS_DESTINATION "APLV1" // APL is LoRa : http://www.aprs.org/aprs11/tocalls.txt --> V1 is a new for this project
#define APRS_SYMBOL '#' // Not used because of WX
#define APRS_SYMBOL_TABLE 'L'
#define APRS_LATITUDE 45.325776
#define APRS_LONGITUDE 5.63658087
#define APRS_ALTITUDE 830
#define APRS_COMMENT "Cam, Meteo, Digi"
#define APRS_STATUS "f4hvv.valentin-saugnier.fr"

#define PIN_WIFI GPIO0
#define PIN_NPR GPIO5
#define PIN_LDR ADC

#ifdef USE_SOFT_SERIAL
#define PIN_PI_RX GPIO2
#define PIN_PI_TX GPIO1
#endif

#define WATCHDOG_TIMEOUT 255
#define WATCHDOG_SAFETY_RESET 600
#define LDR_ALARM_LEVEL 99999 // 1000

#define EEPROM_ADDRESS_WATCHDOG_SAFETY 0
#define EEPROM_ADDRESS_APRS_DIGIPEATER 1
#define EEPROM_ADDRESS_APRS_TELEMETRY 2
#define EEPROM_ADDRESS_APRS_POSITION 3
#define EEPROM_ADDRESS_VERSION 0xFF
#define EEPROM_VERSION 1

#define COLOR_RED 0x500000
#define COLOR_VIOLET 0x500050
#define COLOR_BLUE 0x000050
#define COLOR_YELLOW 0xcfcf02
#define COLOR_GREEN 0x005000
#define COLOR_CYAN 0x00bbff

#ifdef USE_SOFT_SERIAL
#include <softSerial.h>
extern softSerial *SerialPi;
#else
extern HardwareSerial *SerialPi;
#endif
extern JsonWriter serialJsonWriter;
extern char bufferText[255];

#endif //CUBECELL_MONITORING_CONFIG_H

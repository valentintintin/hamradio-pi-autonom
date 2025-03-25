#ifndef RP2040_LORA_APRS_CONFIG_H
#define RP2040_LORA_APRS_CONFIG_H

#define BUFFER_LENGTH 1024

#define LORA_SCK 14  // GPIO14
#define LORA_MISO 24 // GPIO24
#define LORA_MOSI 15 // GPIO15
#define LORA_CS 13   // GPIO13
#define LORA_RESET 23         // GPIO23
#define LORA_BUSY 18          // GPIO18
#define LORA_DIO1 16          // GPIO16
#define LORA_DIO4 17          // GPIO17

#define LORA_PREAMBLE_LENGTH 8
#define TRX_BUFFER 253 // 256 - 3 because 3 bytes for LoRa APRS

#define DISABLE_SLOW_CLOCK false
#define MAX_GPIO_USED 10
#define CALLSIGN_ALERT_MESSAGE_TO "F4HVV-7"

#define TIME_SET_MPPT_WATCHDOG_DFU 120000 // 2 minutes
#define INTERVAL_BLINKER 1000
#define INTERVAL_PRINT_JSON_USB 30000
#define TIME_AFTER_BOOT 90000 // 1 minute 30
#define TIME_WAIT_TOGGLE_WATCHDOG_MASTER 5000 // 5 seconds
#define TIME_BEFORE_REBOOT 5000 // 5 seconds
#define TIME_WAIT_CHANNEL_ACTIVE 1000
#define TIME_AFTER_TX 2500

#define ENERGY_ADC_BATTERY_SENSE_SAMPLES 15
//  ratio of voltage divider = 3.0 (R17=200k, R18=100k)
#define ENERGY_ADC_BATTERY_SENSE_RESOLUTION_BITS ADC_RESOLUTION
#define ENERGY_ADC_MULTIPLIER 3.1 // 3.0 + a bit for being optimistic
#define AREF_VOLTAGE 3.3

extern char bufferText[BUFFER_LENGTH];
extern uint8_t buffer[BUFFER_LENGTH];

#endif


/*
 * Arduino log : format
* %s	display as string (char*)
* %S    display as string from flash memory (__FlashStringHelper* or char[] PROGMEM)
* %c	display as single character
* %C    display as single character or as hexadecimal value (prefixed by `0x`) if not a printable character
* %d	display as integer value
* %l	display as long value
* %u	display as unsigned long value
* %x	display as hexadecimal value
* %X	display as hexadecimal value prefixed by `0x` and leading zeros
* %b	display as binary number
* %B	display as binary number, prefixed by `0b`
* %t	display as boolean value "t" or "f"
* %T	display as boolean value "true" or "false"
* %D,%F display as double value
* %p    display a  printable object
 */
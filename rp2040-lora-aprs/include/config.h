#pragma once

#define MAX_GPIO_USED 16
#define BUFFER_LENGTH 255

#define LORA_SCK 14  // GPIO14
#define LORA_MISO 24 // GPIO24
#define LORA_MOSI 15 // GPIO15
#define LORA_CS 13   // GPIO13
#define LORA_RESET 23         // GPIO23
#define LORA_BUSY 18          // GPIO18
#define LORA_DIO1 16          // GPIO16
#define LORA_DIO4 17          // GPIO17
#define LORA_DIO2_AS_RF_SWITCH
#define LORA_QUEUE_RX_SIZE 20
#define LORA_QUEUE_TX_SIZE 20
#define LORA_PREAMBLE_LENGTH 8
#define TRX_BUFFER 256

#define SETTINGS_VERSION 1
#define DISABLE_SLOW_CLOCK false

#define TIME_SET_MPPT_WATCHDOG_DFU 120000 // 2 minutes
#define INTERVAL_BLINKER 1000 // 1 second
#define INTERVAL_PRINT_JSON_USB 30000 // 30 seconds
#define TIME_AFTER_BOOT 90000 // 1 minute 30
#define TIME_WAIT_TOGGLE_WATCHDOG_MASTER 5000 // 5 seconds
#define TIME_BEFORE_REBOOT 10000 // 10 seconds

#define ENERGY_ADC_BATTERY_SENSE_SAMPLES 15
//  ratio of voltage divider = 3.0 (R17=200k, R18=100k)
#define ENERGY_ADC_BATTERY_SENSE_RESOLUTION_BITS ADC_RESOLUTION
#define ENERGY_ADC_MULTIPLIER 3.1 // 3.0 + a bit for being optimistic
#define AREF_VOLTAGE 3.3

#define CC1101_RECEIVER_CS 99
#define CC1101_RECEIVER_IRQ 99
#define CC1101_RECEIVER_GPIO 99
#define WH65B_MAX_PAYLOAD_LENGTH 27

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
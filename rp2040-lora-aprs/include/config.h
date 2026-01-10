#pragma once

#define BUFFER_LENGTH 256

#define LORA_SCK    14
#define LORA_MISO   12
#define LORA_MOSI   15
#define LORA_CS     13
#define LORA_RESET  10
#define LORA_BUSY   7
#define LORA_RXEN  6
#define LORA_DIO1   11
#define LORA_DIO2_AS_RF_SWITCH true
#define LORA_DIO3_TCXO_VOLTAGE 1.8
#define LORA_CURRENT_LIMIT 130
#define LORA_RX_BOOSTED_GAIN true

#define DISABLE_SLOW_CLOCK false

#define CC1101_RECEIVER_CS 99
#define CC1101_RECEIVER_IRQ 99
#define CC1101_RECEIVER_GPIO 99
#define WH65B_MAX_PAYLOAD_LENGTH 27

#define I2C_SLAVE_ADDRESS 0x11

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

#define PIN_SERIAL1_TX (0u)
#define PIN_SERIAL1_RX (1u)

#define PIN_SERIAL2_TX (8u)
#define PIN_SERIAL2_RX (9u)

// SPI
#define PIN_SPI0_MISO  (16u)
#define PIN_SPI0_MOSI  (19u)
#define PIN_SPI0_SCK   (18u)
#define PIN_SPI0_SS    (17u)

#define PIN_SPI1_MISO  (12u)
#define PIN_SPI1_MOSI  (15u)
#define PIN_SPI1_SCK   (14u)
#define PIN_SPI1_SS    (13u)

// Wire
#define PIN_WIRE0_SDA  (4u)
#define PIN_WIRE0_SCL  (5u)

#define PIN_WIRE1_SDA  (26u)
#define PIN_WIRE1_SCL  (27u)

*/
